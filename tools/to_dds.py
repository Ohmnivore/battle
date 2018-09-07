#!/usr/bin/env python


# Taken from https://github.com/floooh/oryol/blob/master/tools/texexport.py
def to_dds(src_file_path, dst_file_path, linearGamma, fmt, rgbFmt=None) :
    import subprocess

    ddsTool = 'tools/nvcompress'

    print('=== toDDS: {} => {}:'.format(src_file_path, dst_file_path))

    cmd_line = [ddsTool, '-' + fmt]

    if rgbFmt != None:
        cmd_line.append('-rgbfmt')
        cmd_line.append(rgbFmt)

    if linearGamma:
        cmd_line.append('-tolineargamma')

    cmd_line.append(src_file_path)
    cmd_line.append(dst_file_path)
    subprocess.call(args=cmd_line)


def execute(src_dir, dst_dir):
    import os

    for dir_name, subdir_list, file_list in os.walk(src_dir):
        for file_name in file_list:
            base_file_name, extension = os.path.splitext(file_name)

            if extension == '.png':
                src_file_path = os.path.join(dir_name, file_name)

                dst_file_path = [dst_dir]
                dst_file_path += dir_name.strip(os.sep).split(os.sep)[1:]
                dst_file_path += [base_file_name + '.dds']

                dst_file_path = os.sep.join(dst_file_path)

                to_dds(src_file_path, dst_file_path, False, 'rgb', 'rgba8')


def main(args):
    src_dir = args[1]
    dst_dir = args[2]

    execute(src_dir, dst_dir)


if __name__ == "__main__":
    import sys
    main(sys.argv)
