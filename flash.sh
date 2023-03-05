echo "Flashing ..."

openocd -f interface/stlink.cfg -f target/stm32l0.cfg -c "program build/b-l027cz-lrwan1.elf verify reset exit"

echo "Done."