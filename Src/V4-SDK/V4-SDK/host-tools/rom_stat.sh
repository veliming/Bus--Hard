#!/bin/bash

if [ "${1}" = "" ] || [ ! -d "${1}" ];then
    exit
fi

if [ "${2}" = "" ] || [ ! -d "${2}" ];then
    exit
fi

SRCDIR=${1}
OBJDIR=${2}
OBJALL=$(find ${OBJDIR} -name "*.o" | sed -n '/test/d;/demos/d;/portfiles/d;p')
MODULES="core $(ls components 2>/dev/null)"

if [ "${OBJALL}" = "" ] || [ "${MODULES}" = "" ];then
    exit
fi

printf "\n    | %-5s | %-35s | %14s | %26s |\n" "RATE" "OBJ NAME" "BYTES/TOTAL" "MODULE NAME"

for module in ${MODULES}
do
    MODULE_FILES=$(echo "${OBJALL}" | sed -n "/${module}/p")
    MODULE_SIZE=$(size ${MODULE_FILES} | sed '1d' |awk -F' ' 'BEGIN{total=0}{total+=$1}END{print total}')

    [ "${MODULE_SIZE}" != "0" ] && \
    printf "    |-%-.5s-|-%-.35s-|-%.14s-|-%.26s-|\n" \
        "-----------------------------------------" \
        "-----------------------------------------" \
        "-----------------------------------------" \
        "-----------------------------------------"

    for file in ${MODULE_FILES}
    do
        size ${file} | sed '1d' | \
            awk -v sum="${MODULE_SIZE}" \
                -v module="${module}" \
                -v obj="$(basename ${file})" \
                '{if ($1 != 0) printf("%d %.4s%% %s %-d/%d %s\n", $1, ($1/sum)*100, obj, $1, sum, module);}'
    done \
        | sort -nr \
        | cut -d' ' -f2- \
        | awk '{ printf("    | %-5s | %-35s | %14s | %26s |\n", $1, $2, $3, $4); }'
done

echo -e "\n\n\n"

printf "    | %-5s | %-35s | %-9s | %-9s | %-10s | %-6s |\n" \
    "RATE" "MODULE NAME" "ROM" "RAM" "BSS" "DATA"
printf "    |-%-.5s-|-%-.35s-|-%-.9s-|-%-.9s-|-%-.10s-|-%-.6s-|\n" \
    "-------------" \
    "--------------------------------------------" \
    "-------------" \
    "-------------" \
    "-------------" \
    "-------------"

TOTAL_ROM=$(size ${OBJALL} | sed '1d' | awk -F' ' 'BEGIN{total=0}{total+=$1}END{print total}')

for module in ${MODULES}
do
    MODULE_FILES=$(echo "${OBJALL}" | sed -n "/${module}/p")
    size ${MODULE_FILES} | sed '1d' | \
        awk -v name=${module} -v total_rom=${TOTAL_ROM} '
            BEGIN { sum_rom = sum_ram = sum_bss = sum_data = 0 }
            {
                sum_rom += $1 + $2;
                sum_ram += $2 + $3;
                sum_bss += $3;
                sum_data += $2;
            }
            END {
                rate = sum_rom / total_rom * 100;
                if (rate < 100)
                    printf(" | %.4s%% | %-35s | %-9s | %-9s | %-10s | %-6s |\n", rate, name, sum_rom, sum_ram, sum_bss, sum_data);
                else
                    printf(" |  %.3s%% | %-35s | %-9s | %-9s | %-10s | %-6s |\n", rate, name, sum_rom, sum_ram, sum_bss, sum_data);
            }
        '
done \
    | sort -nr \
    | cut -d' ' -f2- \
    | sed 's!.*!    &!g' \
    | awk -v total_rom=${TOTAL_ROM} '
        BEGIN { sum_rom = sum_ram = sum_bss = sum_data = 0 }
        {
            sum_rom += $6;
            sum_ram += $8;
            sum_bss += $10;
            sum_data += $12;
            print;
        }
        END {
            rate = sum_rom / total_rom * 100;
            printf("    |-------|-------------------------------------|-----------|-----------|------------|--------|\n");
            if (rate < 100)
                printf("    | %.4s%% | %-35s | %-9s | %-9s | %-10s | %-6s |\n", rate, "- IN TOTAL -", sum_rom, sum_ram, sum_bss, sum_data);
            else
                printf("    |  %.3s%% | %-35s | %-9s | %-9s | %-10s | %-6s |\n", rate, "- IN TOTAL -", sum_rom, sum_ram, sum_bss, sum_data);
        }
        '

echo ""

