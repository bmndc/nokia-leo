#!/usr/bin/python
# -*- coding: utf-8 -*-

import json

global globvar

def print_header():
    retstr = "#include \"nsUnicodeScriptCodes.h\"\n"
    return retstr

def print_define():
    retstr = "typedef mozilla::unicode::Script Script;\n"
    return retstr

# define structure for unicode and script language
def print_struct_define():
    retstr = "struct unicode_ext_data\n{\n    uint32_t code;\n    Script language;\n};\n"
    return retstr

def print_arrary_begin():
    retstr = "unicode_ext_data unicode_ext_data_array[] = {\n"
    return retstr

def print_arrary_end():
    retstr = "\n};\n"
    return retstr

# for loop to get json data
def parse_json_file(file):
    global globvar
    with open(file) as data_file:
        data_item = json.load(data_file)
    retstr = ""
    for parameters in data_item:
        i = 0
        lenth = len(data_item[parameters])
        globvar = lenth
        for one_data in data_item[parameters]:
            if(i < (lenth-1) ):
                retstr += "    { "+ one_data['code'] + "," + one_data['script'] +" },\n"
            else:
                retstr += "    { "+ one_data['code'] + "," + one_data['script'] +" }"
            i += 1

    return retstr

def print_arrary_size():
    global globvar
    retstr = "uint32_t font_ext_code_size = "+ str(globvar) +" ;\n"

    return retstr


def genfont(header,filename,name):
    header.write(print_header())
    header.write(print_define())
    header.write(print_struct_define())
    header.write(print_arrary_begin())
    header.write(parse_json_file(filename))
    header.write(print_arrary_end())
    header.write(print_arrary_size())


def main():
    text_file = open("Output.h", "w")
    genfont(text_file,"font_data.json", array_names[0] )
    text_file.close()



array_names = [
  'serviceFontExtendSupport'
]

for n in array_names:
   # Make sure the lambda captures the right string.
   globals()[n] = lambda header, filename, name=n: genfont(header,filename,name)


if __name__ == '__main__':
    main()
