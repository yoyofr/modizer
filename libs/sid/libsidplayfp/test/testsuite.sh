#!/bin/sh

dir=$(dirname $0)

while read line; do
        if [[ $line =~ ^# ]]; then continue; fi
        name=${line%% *}
        echo "Running test $name"
        $dir/test $line
        retcode=$?
        if [[ $retcode -ne 0 ]]; then
                echo "Failed test $name"
                exit $retcode
        fi
done <  $dir/testlist

echo "Success!"
