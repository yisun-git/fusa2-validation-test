#!/usr/bin/env python

#################################################################################
# The bentch mark generator
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
class LogParser(object):
    def __init__(self, log_file, map_file, test_times):
        if not os.path.exists(log_file):
            logging.critical('%s not found!' % log_file)
            sys.exit(-1)
        if not os.path.exists(map_file):
            logging.critical('%s not found!' % map_file)
            sys.exit(-1)
        self.log_file = log_file
        self.map_file = map_file
        self.test_times = test_times
        self.data = []
        self.str_enums = {}
        self.data_pattern = re.compile(r'\[.+\] line=\d+ (\d+)')
        self.average_pattern = re.compile(r'native (.+) average is (\d+)')
        utils.fread_lines(self.map_file, LogParser.add_str_enum, self)

    @staticmethod
    def add_str_enum(line_num, line, self):
        line = line.strip()
        if(len(line) == 0 or line[0] == '#'): # skips empty line or comment
            return
        enum_str = line.split(':', 1)
        if(len(enum_str) < 2):
            logging.warning('failed to parse: %s', enum_str)
            return
        enum = enum_str[0].rstrip()
        string = enum_str[1].lstrip().strip('"').upper()
        if(string not in self.str_enums):
            self.str_enums[string] = enum
            logging.debug('self.str_enums["%s"] = %s', string, enum)

    def generate_code(self, output_file):
        with open(output_file, 'w') as outfile:
            self.outfile = outfile
            outfile.write('/* The cache_bench is auto-generated, don\'t modify it. */\n')
            outfile.write('struct cache_data cache_bench[CACHE_SIZE_TYPE_MAX] = {\n')
            utils.fread_lines(self.log_file, LogParser.parse_line, self)
            outfile.write("};\n")
        logging.critical('"%s" is generated', output_file)

    @staticmethod
    def parse_line(line_num, line, self):
        match = self.data_pattern.match(line)
        if (match and len(match.groups()) > 0):
            self.data.append(int(match.group(1)))
        else:
            match = self.average_pattern.match(line)
            if (not match or len(match.groups()) < 2): # ignores the unknown line
                return

            string = match.group(1).upper()
            average = int(match.group(2))
            if(string not in self.str_enums):
                logging.info('ignore "%s"', string)
                return
            enum = self.str_enums[string]
            self.calc_stdev(enum, average)

    def calc_stdev(self, enum, average):
        var = 0
        sum = 0  # used for checking the average is right or not
        count = 0
        is_calc_diff = True if (len(self.data) >= self.test_times * 2) else False

        while(len(self.data) > 0):
            data = self.data.pop()
            if(count >= self.test_times): continue
            if is_calc_diff: data -= self.data.pop()
            sum += data
            var += pow(data - average, 2)
            count += 1

        if(count < self.test_times):
            logging.critical('Can\'t parse %s, because of the invalid data.', enum)
            return

        avg = int(sum/count)
        stdev = math.sqrt(var/(count-1))
        logging.debug('%s count=%d average=%d avg=%d stdev=%d',
            enum, count, average, avg, stdev)

        if(count == self.test_times and average == avg):
            #[CACHE_L1_READ_UC] = {433796UL, 23647UL},
            self.outfile.write('\t[%s] = {%dUL, %dUL},\n' % (enum, average, stdev))
        else:
            logging.warning('the average(%s) of %s is wrong.', average, enum)

#################################################################################

def main():
    '''
    usage: python mem-cache-tool.py -h
    eg: python mem-cache-tool.py -l log64-mem_cache-native.txt -m mem-cache.map
    '''
    parser = argparse.ArgumentParser(usage='%(prog)s [options]')
    parser.add_argument('-l', '--log-file',    dest='log_file',    required=True,  type=str, help='the log file')
    parser.add_argument('-m', '--map-file',    dest='map_file',    required=True,  type=str, help='the map file')
    parser.add_argument('-o', '--output-file', dest='output_file', required=False, type=str, help='the code file')
    args = parser.parse_args()

    logParser = LogParser(args.log_file, args.map_file, 40)
    logParser.generate_code(args.output_file if args.output_file else 'mem_cache_autogen.c')

if __name__ == '__main__':
    utils.log_init(logging.INFO)
    sys.exit(main())

