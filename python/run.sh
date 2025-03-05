# find all hw*.py in python directory

for file in python/hw*.py; do
    echo "Running $file"
    python $file
done
