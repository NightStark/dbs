

#ï¼?bin/bash
for files in `ls *.h`
do
    mv  $files ns_${files}
done
