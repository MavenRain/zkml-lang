module attributes {llzk.lang = "zkml-lang", llzk.main = !struct.type<@Main>} {
  struct.def @Main {
    struct.member @o0 : !felt.type<"bn254"> {llzk.pub}
    struct.member @o1 : !felt.type<"bn254"> {llzk.pub}
    function.def @compute(%x0: !felt.type<"bn254"> {llzk.pub}, %x1: !felt.type<"bn254"> {llzk.pub}, %w0: !felt.type<"bn254">, %w1: !felt.type<"bn254">, %w2: !felt.type<"bn254">, %w3: !felt.type<"bn254">) -> !struct.type<@Main> {
      %self = struct.new : !struct.type<@Main>
      %0 = felt.mul %w0, %x0 : !felt.type<"bn254">, !felt.type<"bn254">
      %1 = felt.mul %w1, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %2 = felt.add %0, %1 : !felt.type<"bn254">, !felt.type<"bn254">
      %3 = felt.mul %w2, %x0 : !felt.type<"bn254">, !felt.type<"bn254">
      %4 = felt.mul %w3, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %5 = felt.add %3, %4 : !felt.type<"bn254">, !felt.type<"bn254">
      %6 = felt.add %x0, %2 : !felt.type<"bn254">, !felt.type<"bn254">
      %7 = felt.add %x1, %5 : !felt.type<"bn254">, !felt.type<"bn254">
      struct.writem %self[@o0] = %6 : !struct.type<@Main>, !felt.type<"bn254">
      struct.writem %self[@o1] = %7 : !struct.type<@Main>, !felt.type<"bn254">
      function.return %self : !struct.type<@Main>
    }
    function.def @constrain(%self: !struct.type<@Main>, %x0: !felt.type<"bn254"> {llzk.pub}, %x1: !felt.type<"bn254"> {llzk.pub}, %w0: !felt.type<"bn254">, %w1: !felt.type<"bn254">, %w2: !felt.type<"bn254">, %w3: !felt.type<"bn254">) {
      %0 = felt.mul %w0, %x0 : !felt.type<"bn254">, !felt.type<"bn254">
      %1 = felt.mul %w1, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %2 = felt.add %0, %1 : !felt.type<"bn254">, !felt.type<"bn254">
      %3 = felt.mul %w2, %x0 : !felt.type<"bn254">, !felt.type<"bn254">
      %4 = felt.mul %w3, %x1 : !felt.type<"bn254">, !felt.type<"bn254">
      %5 = felt.add %3, %4 : !felt.type<"bn254">, !felt.type<"bn254">
      %6 = felt.add %x0, %2 : !felt.type<"bn254">, !felt.type<"bn254">
      %7 = felt.add %x1, %5 : !felt.type<"bn254">, !felt.type<"bn254">
      %8 = struct.readm %self[@o0] : !struct.type<@Main>, !felt.type<"bn254">
      %9 = struct.readm %self[@o1] : !struct.type<@Main>, !felt.type<"bn254">
      constrain.eq %8, %6 : !felt.type<"bn254">, !felt.type<"bn254">
      constrain.eq %9, %7 : !felt.type<"bn254">, !felt.type<"bn254">
      function.return
    }
  }
}
