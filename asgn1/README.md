..............................
ADDITIONAL TESTS: 

************************
get_empty_file.sh:
************************
#!/usr/bin/env bash

# This script performs a get command on an empty file.

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ check_dir -eq 1 ]]; then
    exit 1
fi

## To simplify cleanup, add all files that your script uses.
### You can do this ahead of time, or as you create them.
new_files=""

## Create an empty file
touch empty.txt

## Create command file
echo "get empty.txt" > command
new_files="$new_files empty.txt command"

## run get command and save to file:
cat command | ./memory > output.txt 2>err.log
mem_rc=$?
new_files="$new_files output.txt err.log"

## Check each of the outputs:
## (1) memory's return code is 0
## (2) memory's stdout is empty
## If any of these fail, produce a helpful message and set rc=1

### Note: we don't check for stderr being empty; you can use it for
### logging should you like!

rc=0
msg=""

## check if memory's return code is 0
if [[ $mem_rc -ne 0 ]]; then
    msg=$'Memory returned non-zero error code\n'
    rc=1
fi

if [[ -s output.txt ]]; then
    msg=$"${msg}Memory produced the wrong output to stdout"$'\n'
    rc=1
fi

if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "return code:"
    echo $mem_rc
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat err.log
    echo "--------------------------------------------------------------------------------"
    if [[ -s output.txt ]]; then
        echo "stdout is not empty."
    else
        echo "stdout is empty."
    fi
    echo "--------------------------------------------------------------------------------"
fi

cleanup $new_files
exit $rc


*************************
iget_missing_filename.sh
*************************
#!/usr/bin/env bash

## This script performs a simple get request without a filename.

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ check_dir -eq 1 ]]; then
    exit 1
fi

## To simplify cleanup, add all files that your script uses.
### You can do this ahead of time, or as you create them.
new_files=""

## run get command without a filename and save to file:
echo "get" | ./memory > output.txt 2>err.log
mem_rc=$?
new_files="$new_files output.txt err.log"

## Check each of the outputs:
## (1) memory's return code is 1
## (2) memory's stdout is empty
## (3) memory's stderr is the expected value
## If any of these fail, produce a helpful message and set rc=1

rc=0
msg=""

## check if memory's return code is 1
if [[ $mem_rc -ne 1 ]]; then
    msg=$'Memory returned an incorrect error code\n'
    rc=1
fi

if [[ `wc -c < output.txt ` -ne 0 ]]; then
    msg=$"${msg}Memory produced the wrong output to stdout"$'\n'
    rc=1
fi

## check if memory's stderr is the expected value (note: echo adds an
## '\n' automatically)
diff err.log <(echo "Invalid Command") > diff.txt
diff_rc=$?
if [[ $diff_rc -ne 0 ]]; then
    msg=$"${msg}Memory produced the wrong stderr"$'\n'
    rc=1
fi
new_files="$new_files diff.txt"

## Produce pretty output :-)
if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "return code:"
    echo $mem_rc
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat err.log
    echo "--------------------------------------------------------------------------------"
    if [[ $diff_rc -eq 0 ]]; then
        echo "memory produced correct stderr."
    else
        echo "memory's stderr's difference from correct:"
        cat diff.txt
    fi
    echo "--------------------------------------------------------------------------------"
fi

cleanup $new_files
exit $rc


*************************************
iset_missing_newline.sh
*************************************
#!/usr/bin/env bash

## This script performs a simple set request without a newline.

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ check_dir -eq 1 ]]; then
    exit 1
fi

## To simplify cleanup, add all files that your script uses.
### You can do this ahead of time, or as you create them.
new_files=""

## Create a small ascii file
cp test_files/small_ascii.txt test.txt
new_files="$new_files test.txt"

## run set command without newline between command and content:
printf "set test.txt" | ./memory > output.txt 2>err.log
mem_rc=$?
new_files="$new_files output.txt err.log"

## Check each of the outputs:
## (1) memory's return code is 1
## (2) memory's stdout is empty
## (3) memory's stderr is the expected value
## If any of these fail, produce a helpful message and set rc=1

rc=0
msg=""

## check if memory's return code is 1
if [[ $mem_rc -ne 1 ]]; then
    msg=$'Memory returned an incorrect error code\n'
    rc=1
fi

if [[ `wc -c < output.txt ` -ne 0 ]]; then
    msg=$"${msg}Memory produced the wrong output to stdout"$'\n'
    rc=1
fi

## check if memory's stderr is the expected value (note: echo adds an
## '\n' automatically)
diff err.log <(echo "Invalid Command") > diff.txt
diff_rc=$?
if [[ $diff_rc -ne 0 ]]; then
    msg=$"${msg}Memory produced the wrong stderr"$'\n'
    rc=1
fi
new_files="$new_files diff.txt"

## Produce pretty output :-)
if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "return code:"
    echo $mem_rc
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat err.log
    echo "--------------------------------------------------------------------------------"
    if [[ $diff_rc -eq 0 ]]; then
        echo "memory produced correct stderr."
    else
        echo "memory's stderr's difference from correct:"
        cat diff.txt
    fi
    echo "--------------------------------------------------------------------------------"
fi

cleanup $new_files
exit $rc


******************************
iset_too_long.sh
******************************
#!/usr/bin/env bash

## This script performs a simple set request with an overly long filename.

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ check_dir -eq 1 ]]; then
    exit 1
fi

## To simplify cleanup, add all files that your script uses.
### You can do this ahead of time, or as you create them.
new_files=""

## Get the smallest named file that does not work. This has the added
## benefit of setting "traps" for a program that uses an
## insufficiently long buffer and treats a partial location as a file.

base="a"
2>/dev/null echo "$base" > "$base"
rc=$?

while [[ $rc -eq 0 ]]; do
    new_files="$new_files $base"
    base="$base"a
    2>/dev/null echo "$base" > "$base"
    rc=$?
done

## Create a test file with some content
echo "Test content for too long filename" > test_content.txt
new_files="$new_files test_content.txt"

## run set command:
echo "set $base < test_content.txt" | ./memory > output.txt 2>err.log
mem_rc=$?
new_files="$new_files output.txt err.log"

## Check each of the outputs:
## (1) memory's return code is 1
## (2) memory's stdout is empty
## (3) memory's stderr is the expected value
## If any of these fail, produce a helpful message and set rc=1

rc=0
msg=""

## check if memory's return code is 1
if [[ $mem_rc -ne 1 ]]; then
    msg=$'Memory returned an incorrect error code\n'
    rc=1
fi

if [[ `wc -c < output.txt ` -ne 0 ]]; then
    msg=$"${msg}Memory produced the wrong output to stdout"$'\n'
    rc=1
fi

## check if memory's stderr is the expected value (note: echo adds an
## '\n' automatically)
diff err.log <(echo "Invalid Command") > diff.txt
diff_rc=$?
if [[ $diff_rc -ne 0 ]]; then
    msg=$"${msg}Memory produced the wrong stderr"$'\n'
    rc=1
fi
new_files="$new_files diff.txt"

## Produce pretty output :-)
if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "return code:"
    echo $mem_rc
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat err.log
    echo "--------------------------------------------------------------------------------"
    if [[ $diff_rc -eq 0 ]]; then
        echo "memory produced correct stderr."
    else
        echo "memory's stderr's difference from correct:"
        cat diff.txt
    fi
    echo "--------------------------------------------------------------------------------"
fi

cleanup $new_files
exit $rc

*****************************************
set_invalid_command.sh
*****************************************
#!/usr/bin/env bash

## This script performs a set request with an invalid filename.

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ check_dir -eq 1 ]]; then
    exit 1
fi

## To simplify cleanup, add all files that your script uses.
### You can do this ahead of time, or as you create them.
new_files=""

## Create command file with invalid filename
echo "set /invalid/filename.txt" > command
cat test_files/small_ascii.txt >> command
new_files="$new_files command"

## run set command and save to file:
cat command | ./memory > output.txt 2>err.log
mem_rc=$?
new_files="$new_files output.txt err.log"

## Check each of the outputs:
## (1) memory's return code is 1
## (2) memory's stdout is empty
## (3) memory's stderr is the expected value
## If any of these fail, produce a helpful message and set rc=1

rc=0
msg=""

## check if memory's return code is 1
if [[ $mem_rc -ne 1 ]]; then
    msg=$'Memory returned incorrect error code\n'
    rc=1
fi

if [[ `wc -c < output.txt ` -ne 0 ]]; then
    msg=$"${msg}Memory produced output to stdout when it should be empty"$'\n'
    rc=1
fi

## check if memory's stderr is the expected value (note: echo adds an
## '\n' automatically)
diff err.log <(echo "Invalid Command") > diff.txt
diff_rc=$?
if [[ $diff_rc -ne 0 ]]; then
    msg=$"${msg}Memory produced the wrong stderr"$'\n'
    rc=1
fi
new_files="$new_files diff.txt"

## Produce pretty output :-)
if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "return code:"
    echo $mem_rc
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat err.log
    echo "--------------------------------------------------------------------------------"
    if [[ $diff_rc -eq 0 ]]; then
        echo "memory produced correct stderr."
    else
        echo "memory's stderr's difference from correct:"
        cat diff.txt
    fi
    echo "--------------------------------------------------------------------------------"
fi

cleanup $new_files
exit $rc


*******************************************
set_large_overwrite.sh
*******************************************
#!/usr/bin/env bash

# This script performs a set request to overwrite an existing large binary file.

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ check_dir -eq 1 ]]; then
    exit 1
fi

## To simplify cleanup, add all files that your script uses.
new_files=""

## setup a file to be overwritten
echo "I have a lovely bunch of coconuts" > test.txt

## Create command file
echo "set test.txt" > command
cat test_files/large_binary.dat >> command

new_files="$new_files test.txt command"

## run set command and save to file:
cat command | ./memory > output.txt 2>err.log
mem_rc=$?
new_files="$new_files output.txt err.log"

## Check each of the outputs:
## (1) memory's return code is 0
## (2) memory's stdout is empty
## (3) memory wrote the correct content to test.txt
## If any of these fail, produce a helpful message and set rc=1

rc=0
msg=""

## check if memory's return code is 0
if [[ $mem_rc -ne 0 ]]; then
    msg=$'Memory returned non-zero error code\n'
    rc=1
fi

diff output.txt <(echo "OK") > output_diff.txt
output_diff_rc=$?
if [[  $output_diff_rc -ne 0 ]]; then
    msg=$"${msg}Memory produced the wrong output to stdout"$'\n'
    rc=1
fi
new_files="$new_files output_diff.txt"

diff test.txt test_files/large_binary.dat > diff.txt
diff_rc=$?
if [[ $diff_rc -ne 0 ]]; then
    msg=$"${msg}Memory wrote test.txt incorrectly"$'\n'
    rc=1
fi
new_files="$new_files diff.txt"

if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "return code:"
    echo $mem_rc
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat err.log
    echo "--------------------------------------------------------------------------------"
    if [[ $diff_rc -eq 0 ]]; then
    echo "test.txt is correct."
    else
    echo "test.txt's difference from correct:"
    cat diff.txt
    fi
    echo "--------------------------------------------------------------------------------"
    if [[ $output_diff_rc -eq 0 ]]; then
    echo "stdout is correct."
    else
    echo "stdout's difference from correct:"
    cat output_diff.txt
    fi
    echo "--------------------------------------------------------------------------------"
fi

cleanup $new_files
exit $rc


*****************************************
set_max_filename.sh
*****************************************

#!/usr/bin/env bash

## This script performs a set command with the largest allowed filename

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ check_dir -eq 1 ]]; then
    exit 1
fi

## To simplify cleanup, add all files that your script uses.  You can
## do this ahead of time, or as you create them.
new_files=""

## Create the largest filename that works. This has an added benefit of
## setting "traps" for a program that uses bad buffer sizes.

base=""
2>/dev/null echo "$base"a > "$base"a
rc=$?

while [[ $rc -eq 0 ]]; do
    base="$base"a
    new_files="$new_files $base"
    2>/dev/null echo "$base"a > "$base"a
    rc=$?
done

## Create a test file with some content
echo "Test content for max filename" > test_content.txt
new_files="$new_files test_content.txt"

## run set command:
echo "set $base < test_content.txt" | ./memory > output.txt 2>err.log
mem_rc=$?
new_files="$new_files output.txt err.log"

## Check each of the outputs:
## (1) memory's return code is 0
## (2) memory's stdout is "OK"
## If any of these fail, produce a helpful message and set rc=1

rc=0
msg=""

## check if memory's return code is 0
if [[ $mem_rc -ne 0 ]]; then
    msg=$'Memory returned non-zero error code\n'
    rc=1
fi

## check if memory's stdout is "OK"
diff output.txt <(echo "OK") > diff.txt
diff_rc=$?
if [[ $diff_rc -ne 0 ]]; then
    msg=$"${msg}Memory produced wrong output"$'\n'
    rc=1
fi
new_files="$new_files diff.txt"

if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "return code:"
    echo $mem_rc
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat err.log
    echo "--------------------------------------------------------------------------------"
    if [[ $diff_rc -eq 0 ]]; then
        echo "memory produced correct stdout."
    else
        echo "memory's stdout's difference from correct:"
        cat diff.txt
    fi
    echo "--------------------------------------------------------------------------------"
fi

cleanup $new_files
exit $rc


***********************************
set_partial_write.sh
***********************************
#!/usr/bin/env bash

## This script performs a set request with partial write.

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ check_dir -eq 1 ]]; then
    exit 1
fi

## To simplify cleanup, add all files that your script uses.
### You can do this ahead of time, or as you create them.
new_files=""

## Create a medium binary file:
cp test_files/medium_binary.dat test.dat
new_files="$new_files test.dat"

## Create command file
echo "set test.dat" > command
cat test.dat >> command
new_files="$new_files command"

## run set command and save to file:
cat command | env THROTTLER_WRITES=1 LD_PRELOAD="$source_dir/throttler.so" ./memory > output.txt 2>err.log
mem_rc=$?
new_files="$new_files output.txt err.log"

## Check each of the outputs:
## (1) memory's return code is 0
## (2) memory's stdout is the expected value (OK)
## If any of these fail, produce a helpful message and set rc=1

### Note: we don't check for stderr being empty; you can use it for
### logging should you like!

rc=0
msg=""

## check if memory's return code is 0
if [[ $mem_rc -ne 0 ]]; then
    msg=$'Memory returned non-zero error code\n'
    rc=1
fi

## check if memory's stdout is the expected value (OK)
diff output.txt <(echo "OK") > diff.txt
diff_rc=$?
if [[ $diff_rc -ne 0 ]]; then
    msg=$"${msg}Memory produced wrong output"$'\n'
    rc=1
fi
new_files="$new_files diff.txt"

if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "return code:"
    echo $mem_rc
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat err.log
    echo "--------------------------------------------------------------------------------"
    if [[ $diff_rc -eq 0 ]]; then
        echo "memory produced correct stdout."
    else
        echo "memory's stdout's difference from correct:"
        cat diff.txt
    fi
    echo "--------------------------------------------------------------------------------"
fi

cleanup $new_files
exit $rc



