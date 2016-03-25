#!/bin/bash
rm brute_force_27_perturbed.dat swift_dopair_27_perturbed.dat

./test27cells -p 6 -r 1 -d 0.1 -f perturbed

python difffloat.py brute_force_27_perturbed.dat swift_dopair_27_perturbed.dat 1e-5 5e-6

exit $?
