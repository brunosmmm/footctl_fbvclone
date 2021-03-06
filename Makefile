VHW_INCLUDES=libfbv libpod footctl debug
VHW_CC = gcc
VHW_CFLAGS = -DVIRTUAL_HW -g -Wall $(patsubst %,-I%, . $(VHW_INCLUDES))
VHW_OBJECTS=libfbv/fbv.vhw.o libpod/pod.vhw.o footctl/io.vhw.o footctl/tick.vhw.o footctl/manager.vhw.o footctl/footctl.vhw.o debug/virtual.vhw.o

target_board:
	$(MAKE) -C footctl

dev_board:
	DEVICE=stm32f103c8t6 $(MAKE) -C footctl

clean:
	$(MAKE) -C footctl clean

%.vhw.o: %.c
	$(VHW_CC) $(VHW_CFLAGS) -c $< -o $@

vhw: $(VHW_OBJECTS)
	$(VHW_CC) $(VHW_CFLAGS) -o $@ $^

vhwclean:
	rm -rf $(VHW_OBJECTS) vhw


.PHONY: clean vhwclean
