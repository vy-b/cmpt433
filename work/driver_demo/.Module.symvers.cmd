cmd_/home/vy/cmpt433/work/driver_demo/Module.symvers := sed 's/ko$$/o/' /home/vy/cmpt433/work/driver_demo/modules.order | scripts/mod/modpost -m    -o /home/vy/cmpt433/work/driver_demo/Module.symvers -e -i Module.symvers   -T -