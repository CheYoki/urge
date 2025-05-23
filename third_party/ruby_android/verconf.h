#define RUBY_BASE_NAME			"ruby"
#define RUBY_VERSION_NAME		RUBY_BASE_NAME"-"RUBY_LIB_VERSION
#define RUBY_LIB_VERSION_STYLE		3	/* full */
#define RUBY_EXEC_PREFIX		"/usr/local"
#define RUBY_LIB_PREFIX 		RUBY_EXEC_PREFIX"/lib/ruby"
#define RUBY_ARCH_PREFIX_FOR(arch)	RUBY_LIB_PREFIX"/"arch
#define RUBY_SITEARCH_PREFIX_FOR(arch)	RUBY_LIB_PREFIX"/"arch
#define RUBY_LIB			RUBY_LIB_PREFIX"/"RUBY_LIB_VERSION
#define RUBY_ARCH_LIB_FOR(arch) 	RUBY_LIB"/"arch
#define RUBY_SITE_LIB			RUBY_LIB_PREFIX"/site_ruby"
#define RUBY_SITE_ARCH_LIB_FOR(arch)	RUBY_SITE_LIB2"/"arch
#define RUBY_VENDOR_LIB 		RUBY_LIB_PREFIX"/vendor_ruby"
#define RUBY_VENDOR_ARCH_LIB_FOR(arch)	RUBY_VENDOR_LIB2"/"arch
