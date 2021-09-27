# coding:utf-8

import xlrd
import sys
import os


class TestCase():
    def __init__(self, Title=None, Case_Template=None, Owner=None, Priority=None, Description=None, Component_Value=None,
                 Test_Group=None, Test_Category=None, Execution_Type=None, Code_Scan=None, Test_Platform=None,
                 Test_OS=None, Fusa_Reference=None, Fusa_Feature=None, Test_Project=None, Review_Status=None,
                 TC_Complexity=None, CPU_Operation_Mode=None, Weight=None, Test_Case_Design=None, Formal_Review=None,
                 Requirement_Links=None, Test_Scripts=None, First_Action=None, Save_Action=None, Pre_Condition=None,
                 Search_String=None, Test_Level=None, Test_Strategy_Link=None, Test_Description=None, Test_Input=None):
        self.Title = Title
        self.Case_Template = Case_Template
        self.Owner = Owner
        self.Priority = Priority
        self.Description = Description
        self.Component_Value = Component_Value
        self.Test_Group = Test_Group
        self.Test_Category = Test_Category
        self.Execution_Type = Execution_Type
        self.Code_Scan = Code_Scan
        self.Test_Platform = Test_Platform
        self.Test_OS = Test_OS
        self.Fusa_Reference = Fusa_Reference
        self.Fusa_Feature = Fusa_Feature
        self.Test_Project = Test_Project
        self.Review_Status = Review_Status
        self.TC_Complexity = TC_Complexity
        self.CPU_Operation_Mode = CPU_Operation_Mode
        self.Weight = Weight
        self.Test_Case_Design = Test_Case_Design
        self.Formal_Review = Formal_Review
        self.Requirement_Links = Requirement_Links
        self.Test_Scripts = Test_Scripts
        self.First_Action = First_Action
        self.Save_Action = Save_Action
        self.Pre_Condition = Pre_Condition
        self.Search_String = Search_String
        self.Test_Level = Test_Level
        self.Test_Strategy_Link = Test_Strategy_Link
        self.Test_Description = Test_Description
        self.Test_Input = Test_Input


def get_test_case_list(file_name):
    test_case_list = list()
    path = os.path.join(os.path.split(os.path.realpath(__file__))[0], file_name)
    if os.path.exists(path):
        ExcelFile = xlrd.open_workbook(r'%s' % path, encoding_override='utf-8')
        sheet = ExcelFile.sheet_by_index(0)
        if sheet.ncols != 31:
            print('''pls, use the demo xlsx to execute the script!''')
            sys.exit(1)
        else:
            for i in range(1, sheet.nrows):
                tmp_case = TestCase()
                tmp_case.Title = sheet.cell(i, 0).value.encode('utf-8')
                tmp_case.Case_Template = sheet.cell(i, 1).value.encode('utf-8')
                tmp_case.Owner = sheet.cell(i, 2).value.encode('utf-8')
                tmp_case.Priority = sheet.cell(i, 3).value.encode('utf-8')
                tmp_case.Description = sheet.cell(i, 4).value.encode('utf-8')
                tmp_case.Component_Value = sheet.cell(i, 5).value.encode('utf-8')
                tmp_case.Test_Group = sheet.cell(i, 6).value.encode('utf-8')
                tmp_case.Test_Category = sheet.cell(i, 7).value.encode('utf-8')
                tmp_case.Execution_Type = sheet.cell(i, 8).value.encode('utf-8')
                tmp_case.Code_Scan = sheet.cell(i, 9).value.encode('utf-8')
                tmp_case.Test_Platform = sheet.cell(i, 10).value.encode('utf-8')
                tmp_case.Test_OS = sheet.cell(i, 11).value.encode('utf-8')
                tmp_case.Fusa_Reference = sheet.cell(i, 12).value.encode('utf-8')
                tmp_case.Fusa_Feature = sheet.cell(i, 13).value.encode('utf-8')
                tmp_case.Test_Project = sheet.cell(i, 14).value.encode('utf-8')
                tmp_case.Review_Status = sheet.cell(i, 15).value.encode('utf-8')
                tmp_case.TC_Complexity = sheet.cell(i, 16).value.encode('utf-8')
                tmp_case.CPU_Operation_Mode = sheet.cell(i, 17).value.encode('utf-8')
                tmp_case.Weight = str(sheet.cell(i, 18).value).encode('utf-8')
                tmp_case.Test_Case_Design = sheet.cell(i, 19).value.encode('utf-8')
                tmp_case.Formal_Review = sheet.cell(i, 20).value.encode('utf-8')
                tmp_case.Requirement_Links = str(sheet.cell(i, 21).value).encode('utf-8')
                tmp_case.Test_Scripts = str(sheet.cell(i, 22).value).encode('utf-8')
                tmp_case.First_Action = sheet.cell(i, 23).value.encode('utf-8')
                tmp_case.Save_Action = sheet.cell(i, 24).value.encode('utf-8')
                tmp_case.Pre_Condition = sheet.cell(i, 25).value.encode('utf-8')
                tmp_case.Search_String = str(sheet.cell(i, 26).value).encode('utf-8')
                tmp_case.Test_Level = str(sheet.cell(i, 27).value).encode('utf-8')
                tmp_case.Test_Strategy_Link = str(sheet.cell(i, 28).value).encode('utf-8')
                tmp_case.Test_Description = str(sheet.cell(i, 29).value).encode('utf-8')
                tmp_case.Test_Input = str(sheet.cell(i, 30).value).encode('utf-8')
                test_case_list.append(tmp_case)
            return test_case_list
    else:
        print('not exist xlsx. pls, put the file into the add.py script path and try again!')
        sys.exit(1)
