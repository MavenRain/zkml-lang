//! zkml-plonky3-seq2: the Plonky3 AIR of the zkml-lang entry `seq2Plonky3`.
//!
//! Printed by plonky3print from the words of the entry (slice Z10c).
//!
//! The layout (D5 (a)): one wide row. The columns are the inputs, the weight
//! entries (the matrices in order, row-major), then the entries of each stage.
//! The public values are the inputs, then the outputs. The constraints have no
//! row selector, so each row of the trace must hold the same values.

use p3_air::{Air, AirBuilder, AirBuilderWithPublicValues, BaseAir};
use p3_baby_bear::BabyBear;
use p3_field::PrimeCharacteristicRing;
use p3_matrix::dense::RowMajorMatrix;
use p3_matrix::Matrix;

/// The count of the columns: 2 inputs, 8 weight entries, 4 stage entries.
pub const WIDTH: usize = 14;

/// The count of the public values: 2 inputs, then 2 outputs.
pub const PUBLIC_COUNT: usize = 4;

/// The count of the rows: a power of two, the least that `prove` accepts at
/// the test config.
pub const HEIGHT: usize = 1;

/// The column of weight entry 0.
pub const FIRST_WEIGHT: usize = 2;

/// The column of entry 0 of stage 1.
pub const FIRST_STAGE: usize = 10;

/// The column of entry 0 of the last stage (output 0).
pub const LAST_STAGE: usize = 12;

/// The row: the inputs, the weights, then the entries of each stage.
pub const ROW: [u32; WIDTH] = [3, 5, 2, 7, 11, 13, 3, 1, 4, 6, 41, 98, 221, 752];

/// The public values: the inputs, then the outputs.
pub const PUBLIC: [u32; PUBLIC_COUNT] = [3, 5, 221, 752];

/// The AIR of `seq2Plonky3`.
#[derive(Clone, Copy, Debug, Default)]
pub struct Seq2Air;

impl<F> BaseAir<F> for Seq2Air {
    fn width(&self) -> usize {
        WIDTH
    }
}

impl<AB: AirBuilderWithPublicValues> Air<AB> for Seq2Air {
    fn eval(&self, builder: &mut AB) {
        let main = builder.main();
        let cells = main
            .row_slice(0)
            .and_then(|row| <&[AB::Var; WIDTH]>::try_from(&*row).ok().cloned())
            .zip(<[AB::PublicVar; PUBLIC_COUNT]>::try_from(builder.public_values()).ok());
        // A row or a public value list of a different length fails here.
        builder.assert_zero(AB::Expr::from_bool(cells.is_none()));
        // AB::Var is Clone (not Copy) at the rev: each use before the last is a clone.
        cells
            .into_iter()
            .for_each(|([c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13], [p0, p1, p2, p3])| {
                // Input i is public value i.
                builder.assert_eq(c0.clone(), p0);
                builder.assert_eq(c1.clone(), p1);
                // Stage 1: entry i is the sum over j of W_0[i][j] times entry j of stage 0.
                builder.assert_eq(c10.clone(), c2 * c0.clone() + c3 * c1.clone());
                builder.assert_eq(c11.clone(), c4 * c0 + c5 * c1);
                // Stage 2: entry i is the sum over j of W_1[i][j] times entry j of stage 1.
                builder.assert_eq(c12.clone(), c6 * c10.clone() + c7 * c11.clone());
                builder.assert_eq(c13.clone(), c8 * c10 + c9 * c11);
                // Output i (entry i of the last stage) is public value 2 + i.
                builder.assert_eq(c12, p2);
                builder.assert_eq(c13, p3);
            });
    }
}

/// The values as BabyBear elements.
pub fn field<const N: usize>(values: [u32; N]) -> [BabyBear; N] {
    values.map(BabyBear::new)
}

/// The trace: `first` is row 0, then HEIGHT - 1 copies of ROW.
pub fn trace(first: &[BabyBear]) -> RowMajorMatrix<BabyBear> {
    RowMajorMatrix::new(
        first
            .iter()
            .copied()
            .chain(core::iter::repeat_n(field(ROW), HEIGHT - 1).flatten())
            .collect(),
        WIDTH,
    )
}

#[cfg(test)]
mod tests {
    use super::{field, trace, Seq2Air, FIRST_STAGE, FIRST_WEIGHT, LAST_STAGE, PUBLIC, ROW};
    use p3_baby_bear::{BabyBear, Poseidon2BabyBear};
    use p3_challenger::DuplexChallenger;
    use p3_commit::ExtensionMmcs;
    use p3_dft::Radix2DitParallel;
    use p3_field::extension::BinomialExtensionField;
    use p3_field::{Field, PrimeCharacteristicRing};
    use p3_fri::{create_test_fri_params, TwoAdicFriPcs};
    use p3_merkle_tree::MerkleTreeMmcs;
    use p3_symmetric::{PaddingFreeSponge, TruncatedPermutation};
    use p3_uni_stark::{prove, verify, StarkConfig};
    use rand::rngs::SmallRng;
    use rand::SeedableRng;

    // The BabyBear config of uni-stark/tests/fib_air.rs at the rev (test
    // values, not a secure config).
    type Val = BabyBear;
    type Perm = Poseidon2BabyBear<16>;
    type MyHash = PaddingFreeSponge<Perm, 16, 8, 8>;
    type MyCompress = TruncatedPermutation<Perm, 2, 8, 16>;
    type ValMmcs =
        MerkleTreeMmcs<<Val as Field>::Packing, <Val as Field>::Packing, MyHash, MyCompress, 8>;
    type Challenge = BinomialExtensionField<Val, 4>;
    type ChallengeMmcs = ExtensionMmcs<Val, Challenge, ValMmcs>;
    type Challenger = DuplexChallenger<Val, Perm, 16, 8>;
    type Dft = Radix2DitParallel<Val>;
    type Pcs = TwoAdicFriPcs<Val, Dft, ValMmcs, ChallengeMmcs>;
    type MyConfig = StarkConfig<Pcs, Challenge, Challenger>;

    fn config() -> MyConfig {
        let mut rng = SmallRng::seed_from_u64(1);
        let perm = Perm::new_from_rng_128(&mut rng);
        let hash = MyHash::new(perm.clone());
        let compress = MyCompress::new(perm.clone());
        let val_mmcs = ValMmcs::new(hash, compress);
        let challenge_mmcs = ChallengeMmcs::new(val_mmcs.clone());
        let fri_params = create_test_fri_params(challenge_mmcs, 0);
        let pcs = Pcs::new(Dft::default(), val_mmcs, fri_params);
        MyConfig::new(pcs, Challenger::new(perm))
    }

    /// Prove the trace with `first` as row 0, then verify the proof. In the
    /// dev profile, `prove` runs the debug check first: a failed constraint
    /// panics there.
    fn prove_and_verify(first: &[BabyBear]) -> Result<(), String> {
        let config = config();
        let public = field(PUBLIC);
        let proof = prove(&config, &Seq2Air, trace(first), &public);
        verify(&config, &Seq2Air, &proof, &public).map_err(|e| format!("verify: {e:?}"))
    }

    /// ROW with the entry in `column` plus one.
    fn bump(column: usize) -> Vec<BabyBear> {
        field(ROW)
            .into_iter()
            .enumerate()
            .map(|(i, v)| if i == column { v + BabyBear::ONE } else { v })
            .collect()
    }

    #[test]
    fn accepts_z7_trace() -> Result<(), String> {
        prove_and_verify(&field(ROW))
    }

    #[test]
    #[should_panic(expected = "values didn't match on row 0")]
    fn rejects_mutated_output() {
        prove_and_verify(&bump(LAST_STAGE)).unwrap_or_default();
    }

    #[test]
    #[should_panic(expected = "values didn't match on row 0")]
    fn rejects_mutated_weight() {
        prove_and_verify(&bump(FIRST_WEIGHT)).unwrap_or_default();
    }

    #[test]
    #[should_panic(expected = "values didn't match on row 0")]
    fn rejects_mutated_stage() {
        prove_and_verify(&bump(FIRST_STAGE)).unwrap_or_default();
    }
}
