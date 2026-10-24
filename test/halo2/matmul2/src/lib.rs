//! zkml-halo2-matmul2: the Halo2 circuit of the zkml-lang entry `matmul2Halo2`.
//!
//! build/halo2print prints this crate from the words of the entry (the
//! `langc eval` output), so do not edit it: print it again.
//!
//! The layout: the advice columns a, b and c, one instance column, one fixed
//! column kc, and the selectors s_add, s_sub, s_mul and s_const. One region has
//! one row for each value number. The instance column holds INPUTS, then
//! OUTPUTS.

use halo2_proofs::{
    circuit::{AssignedCell, Layouter, Region, SimpleFloorPlanner, Value},
    halo2curves::bn256::Fr,
    plonk::{
        Advice, Circuit, Column, ConstraintSystem, Error, Expression, Fixed, Instance, Selector,
    },
    poly::Rotation,
};

/// The circuit size is 2^K rows: the least k with 2^k at or above ROWS plus
/// the 6 rows that MockProver keeps (5 blinding rows and 1 more row).
pub const K: u32 = 5;

/// The count of the value rows.
pub const ROWS: usize = 14;

/// The count of the inputs. The instance rows 0 .. INPUT_COUNT hold the inputs.
pub const INPUT_COUNT: usize = 2;

/// The inputs: 4 limbs of 64 bits each, low limb first.
pub const INPUTS: [[u64; 4]; 2] = [
    [3, 0, 0, 0],
    [5, 0, 0, 0],
];

/// The weights, in weight-number order.
pub const WEIGHTS: [[u64; 4]; 4] = [
    [2, 0, 0, 0],
    [7, 0, 0, 0],
    [11, 0, 0, 0],
    [13, 0, 0, 0],
];

/// The outputs: the last trace stage.
pub const OUTPUTS: [[u64; 4]; 2] = [
    [41, 0, 0, 0],
    [98, 0, 0, 0],
];

/// One row of the region. The row offset is the value number.
#[derive(Clone, Copy, Debug)]
pub enum Row {
    /// c is the instance row i.
    Input(usize),
    /// c is the weight j (private, no gate).
    Weight(usize),
    /// c is the constant k (the gate s_const).
    Const([u64; 4]),
    /// c = value i + value j (the gate s_add).
    Add(usize, usize),
    /// c = value i - value j (the gate s_sub).
    Sub(usize, usize),
    /// c = value i * value j (the gate s_mul).
    Mul(usize, usize),
    /// c is the output o. The instance row INPUT_COUNT + o holds it.
    Out(usize),
}

/// The rows of `matmul2Halo2`, in value-number order.
pub const PROGRAM: [Row; ROWS] = [
    Row::Input(0),
    Row::Input(1),
    Row::Weight(0),
    Row::Weight(1),
    Row::Weight(2),
    Row::Weight(3),
    Row::Mul(2, 0),
    Row::Mul(3, 1),
    Row::Add(6, 7),
    Row::Mul(4, 0),
    Row::Mul(5, 1),
    Row::Add(9, 10),
    Row::Out(0),
    Row::Out(1),
];

/// The eq instructions: each is a copy constraint on the c cells of 2 rows.
pub const EQS: [(usize, usize); 2] = [
    (12, 8),
    (13, 11),
];

/// The columns and the selectors of the D5 layout.
#[derive(Clone, Debug)]
pub struct ExportConfig {
    a: Column<Advice>,
    b: Column<Advice>,
    c: Column<Advice>,
    instance: Column<Instance>,
    kc: Column<Fixed>,
    s_add: Selector,
    s_sub: Selector,
    s_mul: Selector,
    s_const: Selector,
}

/// The circuit: the private witness values (the weights and the outputs).
#[derive(Clone, Debug)]
pub struct ExportCircuit {
    weights: Vec<Value<Fr>>,
    outputs: Vec<Value<Fr>>,
}

type Cell = AssignedCell<Fr, Fr>;

impl ExportCircuit {
    /// The circuit with known weights and outputs.
    pub fn new(weights: &[Fr], outputs: &[Fr]) -> Self {
        Self {
            weights: known(weights),
            outputs: known(outputs),
        }
    }

    fn assign_row(
        &self,
        config: &ExportConfig,
        region: &mut Region<'_, Fr>,
        cells: &[Cell],
        row: Row,
        offset: usize,
    ) -> Result<Cell, Error> {
        match row {
            Row::Input(i) => region.assign_advice_from_instance(
                || "input",
                config.instance,
                i,
                config.c,
                offset,
            ),
            Row::Weight(j) => witness(&self.weights, j)
                .and_then(|w| region.assign_advice(|| "weight", config.c, offset, || w)),
            Row::Const(k) => config
                .s_const
                .enable(region, offset)
                .and_then(|()| {
                    region.assign_fixed(|| "kc", config.kc, offset, || {
                        Value::known(Fr::from_raw(k))
                    })
                })
                .and_then(|_| {
                    region.assign_advice(|| "const", config.c, offset, || {
                        Value::known(Fr::from_raw(k))
                    })
                }),
            Row::Add(i, j) => binary_row(
                config,
                region,
                cells,
                (config.s_add, |x: Fr, y: Fr| x + y),
                (i, j),
                offset,
            ),
            Row::Sub(i, j) => binary_row(
                config,
                region,
                cells,
                (config.s_sub, |x: Fr, y: Fr| x - y),
                (i, j),
                offset,
            ),
            Row::Mul(i, j) => binary_row(
                config,
                region,
                cells,
                (config.s_mul, |x: Fr, y: Fr| x * y),
                (i, j),
                offset,
            ),
            Row::Out(o) => witness(&self.outputs, o)
                .and_then(|v| region.assign_advice(|| "out", config.c, offset, || v)),
        }
    }
}

impl Circuit<Fr> for ExportCircuit {
    type Config = ExportConfig;
    type FloorPlanner = SimpleFloorPlanner;

    fn without_witnesses(&self) -> Self {
        Self {
            weights: unknown(&self.weights),
            outputs: unknown(&self.outputs),
        }
    }

    fn configure(meta: &mut ConstraintSystem<Fr>) -> ExportConfig {
        let config = ExportConfig {
            a: meta.advice_column(),
            b: meta.advice_column(),
            c: meta.advice_column(),
            instance: meta.instance_column(),
            kc: meta.fixed_column(),
            s_add: meta.selector(),
            s_sub: meta.selector(),
            s_mul: meta.selector(),
            s_const: meta.selector(),
        };
        meta.enable_equality(config.a);
        meta.enable_equality(config.b);
        meta.enable_equality(config.c);
        meta.enable_equality(config.instance);
        binary_gate(meta, &config, ("add", config.s_add), |a, b, c| a + b - c);
        binary_gate(meta, &config, ("sub", config.s_sub), |a, b, c| a - b - c);
        binary_gate(meta, &config, ("mul", config.s_mul), |a, b, c| a * b - c);
        meta.create_gate("const", |q| {
            let s = q.query_selector(config.s_const);
            let c = q.query_advice(config.c, Rotation::cur());
            let kc = q.query_fixed(config.kc, Rotation::cur());
            [s * (c - kc)]
        });
        config
    }

    fn synthesize(&self, config: ExportConfig, mut layouter: impl Layouter<Fr>) -> Result<(), Error> {
        let cells = layouter.assign_region(
            || "export",
            |mut region| {
                PROGRAM
                    .iter()
                    .zip(0..)
                    .try_fold(Vec::new(), |cells, (row, offset)| {
                        self.assign_row(&config, &mut region, &cells, *row, offset)
                            .map(|cell| cells.into_iter().chain(Some(cell)).collect())
                    })
                    .and_then(|cells| {
                        EQS.iter()
                            .try_for_each(|(i, j)| {
                                cell_at(&cells, *i).and_then(|left| {
                                    cell_at(&cells, *j).and_then(|right| {
                                        region.constrain_equal(left.cell(), right.cell())
                                    })
                                })
                            })
                            .map(|()| cells)
                    })
            },
        )?;
        PROGRAM
            .iter()
            .zip(cells.iter())
            .try_for_each(|(row, cell)| match row {
                Row::Out(o) => {
                    layouter.constrain_instance(cell.cell(), config.instance, INPUT_COUNT + o)
                }
                Row::Input(_)
                | Row::Weight(_)
                | Row::Const(_)
                | Row::Add(..)
                | Row::Sub(..)
                | Row::Mul(..) => Ok(()),
            })
    }
}

type Poly = fn(Expression<Fr>, Expression<Fr>, Expression<Fr>) -> Expression<Fr>;

fn binary_gate(
    meta: &mut ConstraintSystem<Fr>,
    config: &ExportConfig,
    gate: (&'static str, Selector),
    poly: Poly,
) {
    let (name, selector) = gate;
    meta.create_gate(name, |q| {
        let s = q.query_selector(selector);
        let a = q.query_advice(config.a, Rotation::cur());
        let b = q.query_advice(config.b, Rotation::cur());
        let c = q.query_advice(config.c, Rotation::cur());
        [s * poly(a, b, c)]
    });
}

fn binary_row(
    config: &ExportConfig,
    region: &mut Region<'_, Fr>,
    cells: &[Cell],
    gate: (Selector, fn(Fr, Fr) -> Fr),
    operands: (usize, usize),
    offset: usize,
) -> Result<Cell, Error> {
    let (selector, op) = gate;
    let (i, j) = operands;
    let left = cell_at(cells, i)?;
    let right = cell_at(cells, j)?;
    selector.enable(region, offset)?;
    let a = left.copy_advice(|| "a", region, config.a, offset)?;
    let b = right.copy_advice(|| "b", region, config.b, offset)?;
    let value = a
        .value()
        .copied()
        .zip(b.value().copied())
        .map(|(x, y)| op(x, y));
    region.assign_advice(|| "c", config.c, offset, || value)
}

fn cell_at(cells: &[Cell], n: usize) -> Result<&Cell, Error> {
    cells.get(n).ok_or(Error::Synthesis)
}

fn witness(values: &[Value<Fr>], n: usize) -> Result<Value<Fr>, Error> {
    values.get(n).copied().ok_or(Error::Synthesis)
}

fn known(values: &[Fr]) -> Vec<Value<Fr>> {
    values.iter().copied().map(Value::known).collect()
}

fn unknown(values: &[Value<Fr>]) -> Vec<Value<Fr>> {
    values.iter().map(|_| Value::unknown()).collect()
}

/// The field elements of limb rows (canonical by the D3 check).
pub fn field(limbs: &[[u64; 4]]) -> Vec<Fr> {
    limbs.iter().copied().map(Fr::from_raw).collect()
}

/// The instance column: the inputs, then the outputs.
pub fn instance(inputs: &[Fr], outputs: &[Fr]) -> Vec<Fr> {
    inputs.iter().chain(outputs).copied().collect()
}

#[cfg(test)]
mod tests {
    use super::{field, instance, ExportCircuit, INPUTS, K, OUTPUTS, WEIGHTS};
    use halo2_proofs::{arithmetic::Field, dev::MockProver, halo2curves::bn256::Fr};

    fn verify(weights: &[Fr], outputs: &[Fr]) -> Result<(), String> {
        MockProver::run(
            K,
            &ExportCircuit::new(weights, outputs),
            vec![instance(&field(&INPUTS), outputs)],
        )
        .map_err(|e| format!("run: {e:?}"))
        .and_then(|prover| {
            prover
                .verify()
                .map_err(|failures| format!("verify: {failures:?}"))
        })
    }

    fn bump_first(values: &[Fr]) -> Vec<Fr> {
        values
            .first()
            .map(|v| *v + Fr::ONE)
            .into_iter()
            .chain(values.iter().skip(1).copied())
            .collect()
    }

    fn rejects(verdict: Result<(), String>) -> Result<(), String> {
        verdict.map_or_else(
            |_| Ok(()),
            |()| Err("verify accepted a mutated witness".to_string()),
        )
    }

    #[test]
    fn accepts_z7_witness() -> Result<(), String> {
        verify(&field(&WEIGHTS), &field(&OUTPUTS))
    }

    #[test]
    fn rejects_mutated_output() -> Result<(), String> {
        rejects(verify(&field(&WEIGHTS), &bump_first(&field(&OUTPUTS))))
    }

    #[test]
    fn rejects_mutated_weight() -> Result<(), String> {
        rejects(verify(&bump_first(&field(&WEIGHTS)), &field(&OUTPUTS)))
    }
}
