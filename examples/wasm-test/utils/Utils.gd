extends Object
class_name Utils

static func comparable(o):
	return o.hash() if (o is Dictionary or o is Object) else o

static func make_regex(pattern: String) -> RegEx:
	var regex = RegEx.new()
	assert(regex.compile(pattern) == OK, "Invalid regex pattern: %s" % pattern)
	return regex
