// A mutant of test/llzk/matmul2.mlir for `make zk-check` (D11).
// Line 29 names the bn254 value %1 with the type !felt.type<"babybear">.
// llzk-opt must reject it: the MLIR parser sees two types for %1.
// It is not in test/llzk, so the golden rows of test/gate.sh do not read it.
module attributes {llzk.lang = "zkml-lang", llzk.main = !struct.type<@Main>} {
  struct.def @Main {
    struct.member @o0 : !felt.type<"bn254"> {llzk.pub}
    struct.member @o1 : !felt.type<"bn254"> {llzk.pub}
    function.def @compute(%x0: !felt.type<"bn254"> {llzk.pub}, %x1: !felt.type<"bn254"> {llzk.pub},
                          %w0: !felt.type<"bn254">, %w1: !felt.type<"bn254">,
                          %w2: !felt.type<"bn254">, %w3: !felt.type<"bn254">) -> !struct.type<@Main> {
      %self = struct.new : !struct.type<@Main>
      %0 = felt.mul %w0, %x0 : !felt.type<"bn254">, !felt.type<"bn254">
      %1 = felt.mul %w1, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %2 = felt.add %0, %1 : !felt.type<"bn254">, !felt.type<"bn254">
      %3 = felt.mul %w2, %x0 : !felt.type<"bn254">, !felt.type<"bn254">
      %4 = felt.mul %w3, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %5 = felt.add %3, %4 : !felt.type<"bn254">, !felt.type<"bn254">
      struct.writem %self[@o0] = %2 : !struct.type<@Main>, !felt.type<"bn254">
      struct.writem %self[@o1] = %5 : !struct.type<@Main>, !felt.type<"bn254">
      function.return %self : !struct.type<@Main>
    }
    function.def @constrain(%self: !struct.type<@Main>,
                            %x0: !felt.type<"bn254"> {llzk.pub}, %x1: !felt.type<"bn254"> {llzk.pub},
                            %w0: !felt.type<"bn254">, %w1: !felt.type<"bn254">,
                            %w2: !felt.type<"bn254">, %w3: !felt.type<"bn254">) {
      %0 = felt.mul %w0, %x0 : !felt.type<"bn254">, !felt.type<"bn254">
      %1 = felt.mul %w1, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %2 = felt.add %0, %1 : !felt.type<"bn254">, !felt.type<"babybear">
      %3 = felt.mul %w2, %x0 : !felt.type<"bn254">, !felt.type<"bn254">
      %4 = felt.mul %w3, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %5 = felt.add %3, %4 : !felt.type<"bn254">, !felt.type<"bn254">
      %6 = struct.readm %self[@o0] : !struct.type<@Main>, !felt.type<"bn254">
      %7 = struct.readm %self[@o1] : !struct.type<@Main>, !felt.type<"bn254">
      constrain.eq %6, %2 : !felt.type<"bn254">, !felt.type<"bn254">
      constrain.eq %7, %5 : !felt.type<"bn254">, !felt.type<"bn254">
      function.return
    }
  }
}
