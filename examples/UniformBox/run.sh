#!/bin/bash

# Generate the initial conditions if they are not present.
if [ ! -e uniformBox.hdf5 ]
then
    echo "Generating initial conditions for the uniform box example..."
    python makeIC.py 100
fi

../swift -s -t 16 uniformBox.yml
