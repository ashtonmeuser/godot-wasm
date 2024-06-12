extends Object
class_name Utils

static func comparable(o):
	return o.hash() if ((o is Dictionary or o is Object) and o != null) else o

static func make_regex(pattern: String) -> RegEx:
	var regex = RegEx.new()
	var error = regex.compile(pattern)
	assert(error == OK, "Invalid regex pattern: %s" % pattern)
	return regex

static func file_length(f: FileAccess) -> int:
	return f.get_length()

static func to_utf8(s: String) -> PackedByteArray:
	return s.to_utf8_buffer()
