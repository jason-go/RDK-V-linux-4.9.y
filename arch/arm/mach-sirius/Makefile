#
# Makefile for the linux kernel.
#

obj-y					:= sirius.o
obj-$(CONFIG_SMP)	+= headsmp.o platsmp.o
obj-$(CONFIG_SIRIUS_SUSPEND)	+= pm.o self-refresh.o
