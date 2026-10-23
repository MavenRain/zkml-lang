module attributes {llzk.lang = "zkml-lang", llzk.main = !struct.type<@Main>} {
  struct.def @Main {
    function.def @compute() -> !struct.type<@Main> {
      %self = struct.new : !struct.type<@Main>
      %0 = felt.const 0 : !felt.type<"bn254">
      %1 = felt.const 1 : !felt.type<"bn254">
      %2 = felt.const 21888242871839275222246405745257275088548364400416034343698204186575808495616 : !felt.type<"bn254">
      %3 = felt.const 4294967296 : !felt.type<"bn254">
      function.return %self : !struct.type<@Main>
    }
    function.def @constrain(%self: !struct.type<@Main>) {
      %0 = felt.const 0 : !felt.type<"bn254">
      %1 = felt.const 1 : !felt.type<"bn254">
      %2 = felt.const 21888242871839275222246405745257275088548364400416034343698204186575808495616 : !felt.type<"bn254">
      %3 = felt.const 4294967296 : !felt.type<"bn254">
      function.return
    }
  }
}
