

#ifndef HAVE_JSON_OBJECT_NEW_ARRAY_EXT
  #define json_object_new_array_ext(size) json_object_new_array();
#endif

#ifndef HAVE_JSON_OBJECT_OBJECT_ADD_EX
  #define json_object_object_add_ex(obj, key, value, opts) json_object_object_add(obj, key, value);
#endif

/* Work-around for older versions of json-c */
#ifndef JSON_C_OBJECT_ADD_KEY_IS_NEW
#define JSON_C_OBJECT_ADD_KEY_IS_NEW 0
#endif

#ifndef JSON_C_OBJECT_KEY_IS_CONSTANT
#define JSON_C_OBJECT_KEY_IS_CONSTANT 0
#endif

