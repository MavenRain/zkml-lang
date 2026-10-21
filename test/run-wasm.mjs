// Runs entries of a langc Wasm module and prints each result as `langc eval`
// does: a number, or `trap` when the module traps.
// Usage: node test/run-wasm.mjs FILE NAME [ARGS...]
//        node test/run-wasm.mjs FILE - [ABI]   (one `NAME [ARGS...]` line on stdin per call)
// ABI is a `langc abi` dump. For a `NAME words` entry the result is the
// address of a words block [n, w0 .. w(n-1)] in the export `$memory`: the
// script prints the n words with one space between them, or `nil`.
import { readFileSync } from "node:fs";

const [file, name, ...args] = process.argv.slice(2);
if (file === undefined || name === undefined) {
  process.stderr.write("usage: node test/run-wasm.mjs FILE NAME [ARGS...] | FILE -\n");
  process.exit(2);
}
const instance = new WebAssembly.Instance(new WebAssembly.Module(readFileSync(file)), {});

const wordEntries = new Set(name === "-" && args[0] !== undefined
  ? readFileSync(args[0], "utf8").split("\n").filter((line) => line.endsWith(" words")).map((line) => line.split(" ")[0])
  : []);

const wordsAt = (address) => {
  const view = new DataView(instance.exports["$memory"].buffer);
  const at = (k) => view.getBigUint64(Number(address) + 8 * k, true);
  const n = Number(at(0));
  return n === 0 ? "nil" : Array.from({ length: n }, (_, k) => at(k + 1).toString()).join(" ");
};

const call = ([entry, ...words]) => {
  const fn = instance.exports[entry];
  if (typeof fn !== "function") throw new Error(`no exported entry '${entry}'`);
  try {
    const result = BigInt.asUintN(64, fn(...words.map(BigInt)));
    return wordEntries.has(entry) ? wordsAt(result) : result.toString();
  } catch (e) {
    if (e instanceof WebAssembly.RuntimeError) return "trap";
    throw e;
  }
};

const calls = name === "-"
  ? readFileSync(0, "utf8").split("\n").filter((line) => line !== "").map((line) => line.split(" "))
  : [[name, ...args]];
process.stdout.write(calls.map((c) => call(c) + "\n").join(""));
