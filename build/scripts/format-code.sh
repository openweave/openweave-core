#!/bin/sh

SED=/bin/sed
PERL=/usr/bin/perl
CLANGFORMAT=clang-format-9

FILENAME=${1}

# do anything iff we have clang-format
if ! clang-format -version; then
  echo "Missing clang-format, please install it prior to reformatting the code"
  exit 1
fi
# clang format has difficulty recognizing the
# WeaveMakeManagedNamespaceIdentifier() macro as a valid namespace;
# temporarily change the namespace name to something more palatable to
# clang-format

sed  -i -e 's:^[ 	]*namespace[ 	]\+WeaveMakeManagedNamespaceIdentifier(\([^ 	,()]\+\),[ 	]*\([^ 	,()]\+\))[ 	]*{[ 	]*$:namespace WeaveMakeManagedNamespaceIdentifier_\1_\2 {:g' ${FILENAME}

${CLANGFORMAT} -i -style=file ${FILENAME}

sed -i -e 's:namespace WeaveMakeManagedNamespaceIdentifier_\([^_]\+\)_\([^ ]\+\):namespace WeaveMakeManagedNamespaceIdentifier(\1, \2):g' ${FILENAME}

# clang formatter does not seem to provide an option to insert a space into the empty expression i.e., {}, so we hit this with sed
sed -i -e 's:{}:{ }:g' ${FILENAME}
sed -i -e 's:;):; ):g' ${FILENAME}

# weave prefers to separate the keyword operator from the operator itself
sed -i -e 's:operator\([-+/*|&=~!%<>[(]\):operator \1:g' ${FILENAME}

#space before the opening brace in short functions
sed -i -e 's:){:) {:g' ${FILENAME}
sed -i -e 's:({:( {:g' ${FILENAME}

sed -i -e 's:\(.\)\([^ "@]\){:\1\2 {:g' ${FILENAME}
