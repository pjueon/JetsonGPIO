import os
import shutil

prefix = "/usr/local/"
with open("./install_manifest.txt", "r") as f:
    paths = [l.strip() for l in f.readlines() if l.startswith(prefix)]

destination = "./install"
if not os.path.exists(destination):
    os.makedirs(destination)

for input_path in paths:
    filename = os.path.basename(input_path)
    directory = input_path[len(prefix):-len(filename)]

    directory = os.path.join(destination, directory)
    if not os.path.exists(directory):
        os.makedirs(directory)

    output_path = os.path.join(directory, filename)
    shutil.copy(input_path, output_path)

    print(f"copy '{input_path}' to '{output_path}'")
