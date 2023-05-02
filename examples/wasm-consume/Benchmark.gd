extends RefCounted
class_name Benchmark

static func fibonacci(n: int):
	var f = [0, 1]
	for i in range(2, n + 1):
		f.append(f[i - 1] + f[i - 2])
	return f[n]

static func sieve(limit: int) -> int:
	# Low primes
	if limit <= 0: return 0
	if limit <= 1: return 1
	if limit <= 2: return 2
	if limit <= 4: return 3

	# Initialise the sieve array with False values
	var sieve = []
	sieve.resize(limit + 1)
	sieve.fill(false)

	# Mark sieve[n] is True if one of the following is True:
	# a) n = (4*x*x)+(y*y) has odd number of solutions, i.e., there exist odd number of distinct pairs (x, y) that satisfy the equation and n % 12 = 1 or n % 12 = 5.
	# b) n = (3*x*x)+(y*y) has odd number of solutions and n % 12 = 7
	# c) n = (3*x*x)-(y*y) has odd number of solutions, x > y and n % 12 = 11
	var x = 1
	while x * x <= limit:
		var y = 1
		while y * y <= limit:
			var n = (4 * x * x) + (y * y)
			if (n <= limit and (n % 12 == 1 or n % 12 == 5)):
				sieve[n] = !sieve[n]

			n = (3 * x * x) + (y * y)
			if n <= limit and n % 12 == 7:
				sieve[n] = !sieve[n]

			n = (3 * x * x) - (y * y)
			if (x > y and n <= limit and n % 12 == 11):
				sieve[n] = !sieve[n]
			y += 1
		x += 1

	# Mark all multiples of squares as non-prime
	var r = 5
	while r * r <= limit:
		if sieve[r]:
			for i in range(r * r, limit + 1, r * r):
				sieve[i] = false
		r += 1

	# Return highest prime
	for a in range(limit, 4, -1):
		if sieve[a]: return a

	return 0
