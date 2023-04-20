export const global_const: f64 = 1.6180339; // Example of exported constant
export let from_callback: i64 = 0; // Stores import function return value
export let memory_value: i64; // Used to store first 8 bytes of memory

// Declare imported function
// Optionally rename module and name via @external("module", "name")
// See https://www.assemblyscript.org/concepts.html#code-annotations
declare function callback(a: i64): i64

// Stub AssemblyScript abort method to avoid import function
// See https://www.assemblyscript.org/concepts.html#special-imports
function abort(a: i32, b: i32, c: i32, d: i32): void { }

export function update_memory(): void {
  memory_value = load<i64>(0);
}

// Call imported function from Wasm module
// In practice, this could be called when encountering error, etc.
// Instead, we'll just set a global with the callback return value
export function invoke_callback(): void {
  from_callback = callback(from_callback);
}

export function fibonacci(_n: i64): i64 {
  const n: i32 = <i32>_n;
  let f = new Array<i32>(n + 2);
  f[0] = 0;
  f[1] = 1;
  for (let i = 2; i <= n; i++)
    f[i] = f[i - 1] + f[i - 2];
  return f[n];
}

export function sieve(_limit: i64): i32 {
  // Cast limit to expected type
  const limit: i32 = <i32>_limit;

  // Low primes
	if (limit <= 0) return 0;
	if (limit <= 1) return 1;
	if (limit <= 2) return 2;
	if (limit <= 4) return 3;

  // Initialize the sieve array with false values
  let sieve = new Array<bool>(limit + 1).fill(false);

  // Mark sieve[n] is true if one of the following is true:
  // a) n = (4*x*x)+(y*y) has odd number of solutions, i.e., there exist odd number of distinct pairs (x, y) that satisfy the equation and n % 12 = 1 or n % 12 = 5.
  // b) n = (3*x*x)+(y*y) has odd number of solutions and n % 12 = 7
  // c) n = (3*x*x)-(y*y) has odd number of solutions, x > y and n % 12 = 11
  for (let x = 1; x * x <= limit; x++) {
    for (let y = 1; y * y <= limit; y++) {
      let n = (4 * x * x) + (y * y);
      if (n <= limit && (n % 12 == 1 || n % 12 == 5))
        sieve[n] = !sieve[n];

      n = (3 * x * x) + (y * y);
      if (n <= limit && n % 12 == 7)
        sieve[n] = !sieve[n];

      n = (3 * x * x) - (y * y);
      if (x > y && n <= limit && n % 12 == 11)
        sieve[n] = !sieve[n];
    }
  }

  // Mark all multiples of squares as non-prime
  for (let r = 5; r * r <= limit; r++) {
    if (sieve[r]) {
      for (let i = r * r; i <= limit; i += r * r)
        sieve[i] = false;
    }
  }

  // Return highest prime
  for (let i = limit; i >= 0; i -= 1)
    if (sieve[i]) return i;

  return 0;
}
