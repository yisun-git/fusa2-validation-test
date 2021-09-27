#!/usr/bin/python3
#################################################################################
# The instruction checker
# Author: Yang Bo(box.yang@intel.com)
#################################################################################

import argparse
import logging
import math
import os
import re
import sys

#################################################################################

class utils(object):
    @staticmethod
    def log_init(level=logging.INFO, log2file=False):
        logger = logging.getLogger()
        logger.setLevel(level)
        formatter = logging.Formatter('%(message)s')

        # print to console
        handler = logging.StreamHandler(sys.stderr)
        handler.setFormatter(formatter)
        logger.addHandler(handler)

        if log2file:
            handler = logging.FileHandler('output.log')
            handler.setFormatter(formatter)
            logger.addHandler(handler)

    @staticmethod
    def fread_lines(fname, callback, data):
        assert fname
        assert callback
        with open(fname, 'r') as file:
            line_num = 0
            for line in file:
                line_num += 1
                callback(line_num, line, data)

    @staticmethod
    def fread_all(fname):
        assert fname
        lines = None
        with open(fname, 'r') as file:
            lines = file.readlines()
        return lines

    @staticmethod
    def fwrite(fname, line, mode = 'w'):
        assert fname
        lines = None
        with open(fname, mode) as file:
            file.write(line)


#################################################################################

# Generate the source code
class InstructionParser(object):
    def __init__(self, dump_file, prefix):
        if not os.path.exists(dump_file):
            logging.critical('%s not found!' % dump_file)
            sys.exit(-1)
        self.dump_file = dump_file
        self.prefix = prefix
        self.sym_name = '<none>'
        self.sym_names = []
        self.sym_pattern = re.compile(r'[0-9a-f]{4,}\s+<(\w+)>')
        self.data_pattern = re.compile(r'.+:\s+(\w{2}) ')

    def output(self):
        utils.fread_lines(self.dump_file, InstructionParser.parse_line, self)
        count = len(self.sym_names)
        if (0 == count):
            logging.critical('==== "%s" not found ====', self.prefix)
        else:
            logging.critical('==== %d "%s" found as follows ====\n',
                count, self.prefix)
            while self.sym_names:
                logging.critical('%s', self.sym_names.pop(0))

    @staticmethod
    def parse_line(line_num, line, self):
        match = self.sym_pattern.match(line)
        if (match and len(match.groups()) > 0):
            logging.debug('=> %s', match.group(1))
            self.sym_name = line
        else:
            match = self.data_pattern.match(line)
            if (match and len(match.groups()) > 0):
                logging.debug('=> %s', match.group(1))
                if (self.prefix == match.group(1)):
                    self.sym_names.append(self.sym_name + line)

#################################################################################

def main():
    '''
    usage: python instruction-checker.py -h
    eg: python instruction-checker.py -e mem_cache_native_64.elf -p 67
    '''
    parser = argparse.ArgumentParser(usage='%(prog)s [options]')
    parser.add_argument('-e', '--elf-file', dest='elf_file', required=True, type=str, help='the ELF file')
    parser.add_argument('-p', '--prefix', dest='prefix', required=False, type=str, default='67',
                        help='the prefix need to be checked')
    args = parser.parse_args()

    dump_file = '%s.dump' % args.elf_file
    if 'posix' == os.name:
        dump_file = '/run/user/%s/%s' % (os.geteuid(), os.path.basename(dump_file))
    os.system('objdump -d %s > %s' % (args.elf_file, dump_file))

    instructionParser = InstructionParser(dump_file, args.prefix)
    instructionParser.output()

    if(os.path.exists(dump_file)):
        os.remove(dump_file)

if __name__ == '__main__':
    utils.log_init(logging.INFO)
    sys.exit(main())

