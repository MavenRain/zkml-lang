module attributes {llzk.lang = "zkml-lang", llzk.main = !struct.type<@Main>} {
  struct.def @Main {
    struct.member @o0 : !felt.type<"bn254"> {llzk.pub}
    function.def @compute(%x0: !felt.type<"bn254"> {llzk.pub}, %x1: !felt.type<"bn254"> {llzk.pub}) -> !struct.type<@Main> {
      %self = struct.new : !struct.type<@Main>
      %0 = felt.mul %x0, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      struct.writem %self[@o0] = %0 : !struct.type<@Main>, !felt.type<"bn254">
      function.return %self : !struct.type<@Main>
    }
    function.def @constrain(%self: !struct.type<@Main>, %x0: !felt.type<"bn254"> {llzk.pub}, %x1: !felt.type<"bn254"> {llzk.pub}) {
      %0 = felt.mul %x0, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %1 = struct.readm %self[@o0] : !struct.type<@Main>, !felt.type<"bn254">
      constrain.eq %1, %0 : !felt.type<"bn254">, !felt.type<"bn254">
      function.return
    }
  }
}
