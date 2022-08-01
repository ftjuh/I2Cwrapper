# Create keywords.txt using ctags

.PHONY: default
default : keywords.txt

# remake with all inputs if anyone changes
keywords.txt : $(shell find src -name '*.h')
	echo > $@
	echo "# Classes" >> $@
	ctags -x --c-kinds=cn $^ | egrep -v '^(operator |_)' | awk '{print $$1}' | sort -u | awk 'OFS="\t" {print $$1,"KEYWORD1"}' >> $@
	echo "# Structs" >> $@
	ctags -x --c-kinds=su $^ | egrep -v '^(operator |_)' | awk '{print $$1}' | sort -u | awk 'OFS="\t" {print $$1,"KEYWORD3"}' >> $@
	echo "# Typedefs" >> $@
	ctags -x --c-kinds=t $^ | egrep -v '^(operator |_)' | awk '{print $$1}' | sort -u | awk 'OFS="\t" {print $$1,"KEYWORD1"}' >> $@
	@# v for "global" variables
	echo "# Enums" >> $@
	ctags -x --c-kinds=egv $^ | egrep -v '^(operator |_)' | awk '{print $$1}' | sort -u | awk 'OFS="\t" {print $$1,"LITERAL1"}' >> $@
	echo "# Methods" >> $@
	ctags -x --c-kinds=fp $^ | egrep -v '^(operator |_)' | awk '{print $$1}' | sort -u | awk 'OFS="\t" {print $$1,"KEYWORD1"}' >> $@
#	echo "# #define" >> $@
#	ctags -x --c-kinds=d $^ | egrep -v '^(operator |_)' | awk '{print $$1}' | sort -u | awk 'OFS="\t" {print $$1,"LITERAL1"}' >> $@
	echo "# members" >> $@
	ctags -x --c-kinds=m $^ | egrep -v '^(operator |_)' | awk '{print $$1}' | sort -u | awk 'OFS="\t" {print $$1,"LITERAL2"}' >> $@
	@# too broad? for constants
	@#echo "# constants" >> $@
	@#ctags -x --c-kinds=v $^ | egrep -v '^(operator |_)' | awk '{print $$1}' | sort -u | awk 'OFS="\t" {print $$1,"LITERAL2"}' >> $@

# c  classes
# g  enumeration names
# e  enumerators (values inside an enumeration)
# s  structure names
# u  union names
# t  typedefs
# n  namespaces
# d  macro definitions
# m  class, struct, and union members

# v  variable definitions
# f  function definitions
# p  function prototypes [off]

# l  local variables [off]
# x  external and forward variable declarations [off]

.PHONY: version
version:
	@echo -n "Last tag: "
	@git tag -n | tail -n 1
	@echo '# Use: git tag -m "what is this" n.n.n'
	@echo "# edit library.properties"
	@grep '^version' library.properties

.PHONY: debug
debug: $(shell find src -name '*.h')
	@echo $^ | xargs -n 1 echo
	
