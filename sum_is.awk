#!/usr/bin/awk -f
BEGIN {
    FS=","
}

{
    sum_files=sum_files+$2;
    sum_lines=sum_lines+$3;
}

END {
    printf("%s,%s\n", sum_files, sum_lines)
}
