"""
upload case to rqm
usage: python RQM.py add demo.xlsx True
"""

# coding=utf-8
import time
import sys
import re
from selenium import webdriver
from selenium.webdriver.support.select import Select
import selenium.webdriver.support.ui as ui
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.common.by import By
import selenium.webdriver.support.expected_conditions as EC
from selenium.webdriver.common.keys import Keys
from selenium.webdriver import ActionChains
import logging
import readxlsx


def is_clickable(locator, timeout=10):
    try:
        ui.WebDriverWait(driver, timeout).until(EC.element_to_be_clickable((By.XPATH, locator)))
        return True
    except TimeoutException:
        return False

def is_visible(locator, timeout=10):
    try:
        ui.WebDriverWait(driver, timeout).until(EC.visibility_of_element_located((By.XPATH, locator)))
        return True
    except TimeoutException:
        return False

def selectelement(xpathId,textName):
    xpath="//*[@id='%s']" % (xpathId)
    s = driver.find_element_by_xpath(xpath)
    Select(s).select_by_visible_text(textName)

def try_run(cmd, ele_instance=None, value=None, timeout=120):
    while timeout > 0:
        try:
            if ele_instance is not None:
                if '''clear()''' in cmd:
                    print('run: {}.{}'.format(ele_instance, cmd))
                    output = ele_instance.clear()
                elif '''send_keys()''' in cmd:
                    print('run: {}.send_keys({})'.format(ele_instance, value))
                    output = ele_instance.send_keys(value)
                elif '''Select().select_by_index()''' in cmd:
                    print('run: Select({}).select_by_index({})'.format(ele_instance, value))
                    output = Select(ele_instance).select_by_index(value)
                elif '''Select().select_by_visible_text()''' in cmd:
                    print('run: Select({}).select_by_visible_text({})'.format(ele_instance, value))
                    output = Select(ele_instance).select_by_visible_text(value)
                elif '''click()''' in cmd:
                    print('run: {}.{}'.format(ele_instance, cmd))
                    output = ele_instance.click()
                elif '''find_element_by_xpath()''' in cmd:
                    print('run: {}.find_element_by_xpath({})'.format(ele_instance, value))
                    output = ele_instance.find_element_by_xpath(value)
            elif value is not None:
                if '''driver.execute_script()''' in cmd:
                    output = driver.execute_script(value)
            else:
                print('run: {}'.format(cmd))
                output = eval(cmd)
            time.sleep(3)
            timeout -= 3
        except Exception as e:
            print('error: {}'.format(e))
            time.sleep(3)
            timeout -= 3
            continue
        else:
            break
    return output

def select_template(template_name):
    for _ in range(5):
        if is_visible('//td/div/div/div/div/span/div/div/span[3]', 60):
            template_button = try_run('''driver.find_element_by_xpath('//td/div/div/div/div/span/div/div/span[3]')''')
            try_run('''click()''', template_button)

        if is_clickable('//span[contains(text(),"{}")]'.format(template_name), 30):
            select_template = driver.find_element_by_xpath('//span[contains(text(),"{}")]'.format(template_name))
            select_template.click()
            break
        else:
            time.sleep(2)
    js = "var q=document.documentElement.scrollTop=0"
    driver.execute_script(js)

def get_table_data(content):
    result = []
    content = content.replace('<br/><br/>', '<br/>')
    content = content.strip()
    if content.endswith(r'<br/>'):
        content = content[:-5]
    trs_list = content.split('\n')
    for row in trs_list:
        tds = row.split("|||")
        result.append(tds)
    print(result)
    return result

def replace_str(old_str):
    new_str = old_str.replace('\n', '<br/>').replace('\\', '\\\\').replace('\"', '\\"').replace("\'", "\\'")
    return new_str

def _test_case_summary(title, case_template, owner, priority, description, component_value, test_group, test_category,
                          execution_type, code_scan,test_platform, test_os, fusa_reference, fusa_feature, test_project,
                          review_status, tc_complexity,cpu_operation_mode, weight, first_action, Test_Level, action='update'):
    if not (case_template == "null" or case_template == "" or action == 'update'):
        select_template(case_template)

    # Set first action
    if not (first_action == "null" or first_action == ""):
        action_element = try_run(
            '''driver.find_element_by_xpath("//table/tbody/tr[1]/td[4]/div/span[2]")''')
        try_run('''click()''', action_element)
        time.sleep(3)
        is_complete = try_run('''driver.find_element_by_xpath('//span[contains(text(),"{}")]')'''.format(first_action))
        try_run('''click()''', is_complete)

    ###write test title
    if not (title == "null" or title == ""):
        title_element = try_run(
            '''driver.find_element_by_xpath("//div/div/div[3]/div[2]/div/div[5]/div/span[6]/span[2]/textarea")''')
        try_run('''clear()''', title_element)
        try_run('''send_keys()''', title_element, title)

    # select owner
    if not (owner == "null" or owner == ""):
        owner_pull_button = try_run('''driver.find_element_by_xpath("//table/tbody/tr[2]/td[4]/span/div/span[2]")''')
        try_run('''click()''', owner_pull_button)
        try:
            ele = driver.find_element_by_xpath('//span[contains(text(),"{}")]'.format(owner))
            ele.click()
            # selectelement('com_ibm_asq_common_web_ui_internal_widgets_ContributorSelect_1','%s' % caseOwner)
        except Exception as e:
            print(e)
            driver.find_element_by_xpath('//span[contains(text(),"{}")]'.format('More...')).click()
            # try_run('''selectelement('com_ibm_asq_common_web_ui_internal_widgets_ContributorSelect_1','More...')''')
            owner_element = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div/div[3]/input')''')
            try_run('''send_keys()''', owner_element, owner)
            seowner_element = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div/div[5]/select')''')
            try_run('''Select().select_by_index()''', seowner_element, 0)
            button_element = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div/div[7]/button[1]')''')
            try_run('''click()''', button_element)

    # select priority
    if not (priority == "null" or priority == ""):
        priority_ele = try_run('''driver.find_element_by_xpath("//table/tbody/tr[3]/td[2]/span/div/span[2]")''')
        try_run('''click()''', priority_ele)
        time.sleep(3)
        select_item = try_run('''driver.find_element_by_xpath('//span[contains(text(),"{}")]')'''.format(priority))
        try_run('''click()''', select_item)

    # Input description
    if not (description == "null" or description == ""):
        description_area = try_run('''driver.find_element_by_xpath("//table/tbody/tr/td[2]/div/div/div[2]/textarea")''')
        try_run('''clear()''', description_area)
        try_run('''send_keys()''', description_area, description)

    # switch to container view
    viewcontainer = try_run(
        '''driver.find_element_by_class_name("com-ibm-asq-common-web-ui-collapsible-section-content-outer-container")''')

    if not (component_value == "null" or component_value == ""):
        # select component value
        scom = try_run(
            '''driver.find_element_by_class_name("com-ibm-asq-common-web-ui-collapsible-section-content-outer-container").find_element_by_tag_name("select")''')
        try:
            Select(scom).select_by_visible_text("%s" % component_value)
        except Exception as e:
            print(e)
            try_run('''Select().select_by_visible_text()''', scom, "More...")
            com_input_ele = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div/span/div/div/input')''')
            try_run('''send_keys()''', com_input_ele, component_value)
            component_sel_ele = try_run(
                '''driver.find_element_by_class_name('content-container').find_element_by_xpath('//div/span/div/div[2]/div/div[3]/div/table/tbody/tr/td')''')
            try_run('''click()''', component_sel_ele)
            component_button_ele = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div[2]/div[2]/span[1]/button[1]')''')
            try_run('''click()''', component_button_ele)

    # select test group
    if not (test_group == "null" or test_group == ""):
        test_group_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                     '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Test Group:')]/../../td[2]/div/select''')
        try_run('''Select().select_by_visible_text()''', test_group_element, test_group)

    # select Test Category
    if not (test_category == "null" or test_category == ""):
        click_cat_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                    '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Test Category:')]/../../td[2]/div/div''')
        try_run('''click()''', click_cat_element)
        test_category_list = test_category.split('++')
        test_category_option = ['General','Application Constraint']
        for i in test_category_option:
            now_click_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                        '''//td[contains(text(),'{}')]'''.format(i))
            now_click_element_parent = try_run('''find_element_by_xpath()''', viewcontainer,
                                        '''//td[contains(text(),'{}')]/..'''.format(i))
            item_check_status = now_click_element_parent.get_attribute('aria-checked')
            if item_check_status == 'true':
                if i not in test_category_list:
                    try_run('''click()''', now_click_element)
            else:
                if i in test_category_list:
                    try_run('''click()''', now_click_element)
        try_run('''click()''', click_cat_element)
        
    # select execute type
    if not (execution_type == "null" or execution_type == ""):
        execute_type_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                       '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Execution Type:')]/../../td[2]/div/select''')
        try_run('''Select().select_by_visible_text()''', execute_type_element, execution_type)

    # Select code scan
    if not (code_scan == "null" or code_scan == ""):
        code_scan_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                    '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'CodeScan:')]/../../td[2]/div/select''')
        try_run('''Select().select_by_visible_text()''', code_scan_element, code_scan)

    ###select Test Platform
    if not (test_platform == "null" or test_platform == ""):
        plat_element = try_run('''find_element_by_xpath()''', viewcontainer,
                               '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Test Platform:')]/../../td[2]/div/div''')
        try_run('''click()''', plat_element)
        test_platform_list = test_platform.split('++')
        test_platform_option = ['APL Leafhill', 'APL MRB', 'APL NUC', 'APL UP2', 'KBL Desktop', 'KBL NUC']
        for i in test_platform_option:
            now_click_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                        '''//td[contains(text(),'{}')]'''.format(i))
            now_click_element_parent = try_run('''find_element_by_xpath()''', viewcontainer,
                                               '''//td[contains(text(),'{}')]/..'''.format(i))
            item_check_status = now_click_element_parent.get_attribute('aria-checked')
            if item_check_status == 'true':
                if i not in test_platform_list:
                    try_run('''click()''', now_click_element)
            else:
                if i in test_platform_list:
                    try_run('''click()''', now_click_element)
        try_run('''click()''', plat_element)

        # ##select OS
        if not (test_os == "null" or test_os == ""):
            s_os_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                   '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'OS:')]/../../td[2]/div/div''')
            # try_run('''Select().select_by_visible_text()''', s_os_element, test_os)
            try_run('''click()''', s_os_element)
            test_os_list = test_os.split('++')
            test_os_option = ['Android as a guest', 'Clear Linux as a guest', 'Clear Linux as a non-safety guest',
                              'Clear Linux as a RTVM', 'clear linux as service OS', 'Unassigned', 'VxWorks as a RTVM',
                              'Windows as a guest', 'Xenomai as a RTVM'] # 'Zephyr as a RTVM'  'Zephyr as a safety guest' 去掉了！！！（需要加上）
            for i in test_os_option:
                try:
                    now_click_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                                '''//td[contains(text(),'{}')]'''.format(i), timeout=60)
                    now_click_element_parent = try_run('''find_element_by_xpath()''', viewcontainer,
                                                       '''//td[contains(text(),'{}')]/..'''.format(i), timeout=60)
                    item_check_status = now_click_element_parent.get_attribute('aria-checked')
                    if item_check_status == 'true':
                        if i not in test_os_list:
                            try_run('''click()''', now_click_element_parent)
                    else:
                        if i in test_os_list:
                            try_run('''click()''', now_click_element_parent)
                except Exception as e:
                    print(e)
                    driver.find_element_by_xpath('//td[contains(text(),"{}")]'.format('More...')).click()
                    more_element_select = try_run('''find_element_by_xpath()''', viewcontainer,
                                                  '''//div/table/tbody/tr/td/div[contains(text(),'{}')]/../../td'''.format(i), timeout=60)
                    more_elemet_subelement = try_run('''find_element_by_xpath()''', viewcontainer,
                                                     '''//div/table/tbody/tr/td/div[contains(text(),'{}')]/../../td/span/div/input'''.format(i), timeout=60)
                    item_check_status = more_elemet_subelement.get_attribute('aria-checked')
                    if item_check_status == 'true':
                        if i not in test_os_list:
                            try_run('''click()''', more_element_select)
                    else:
                        if i in test_os_list:
                            try_run('''click()''', more_element_select)
                    # click OK or cancel button.
                    button_status = driver.find_element_by_class_name('content').find_element_by_xpath(
                        '//div/div[2]/div[2]/span[1]/button').is_enabled()
                    if button_status == 'true':
                        component_button_ok = try_run('''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div[2]/div[2]/span[1]/button')''')
                        try_run('''click()''', component_button_ok)
                    else:
                        component_button_cancel = try_run('''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div[2]/div[2]/span[2]/button')''')
                        try_run('''click()''', component_button_cancel)
                    try_run('''click()''', s_os_element)
            try_run('''click()''', s_os_element)

    # Select Fusa Reference
    if not (fusa_reference == "null" or fusa_reference == ""):
        fusa_reference_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                 '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Fusa Reference :')]/../../td[2]/div/div''')
        try_run('''click()''', fusa_reference_element)
        fusa_reference_list = fusa_reference.split('++')
        fusa_reference_option = ['SDM, Vol.1', 'SDM, Vol.2', 'SDM, Vol.3', 'SDM, Vol.4', ]
        for i in fusa_reference_option:
            now_click_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                        '''//td[contains(text(),'{}')]'''.format(i))
            now_click_element_parent = try_run('''find_element_by_xpath()''', viewcontainer,
                                               '''//td[contains(text(),'{}')]/..'''.format(i))
            item_check_status = now_click_element_parent.get_attribute('aria-checked')
            if item_check_status == 'true':
                if i not in fusa_reference_list:
                    try_run('''click()''', now_click_element)
            else:
                if i in fusa_reference_list:
                    try_run('''click()''', now_click_element)
        try_run('''click()''', fusa_reference_element)

    # Select Fusa feature
    if not (fusa_feature == "null" or fusa_feature == ""):
        fusa_feature_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                       '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Fusa Feature:')]/../../td[2]/div/select''')
        try:
            Select(fusa_feature_element).select_by_visible_text(fusa_feature)
        except Exception as e:
            print(e)
            try_run('''Select().select_by_visible_text()''', fusa_feature_element, "More...")
            com_input_ele = try_run(
                '''driver.find_element_by_xpath("//div/div[1]/span/div/div[1]/input")''')
            try_run('''send_keys()''', com_input_ele, fusa_feature)
            component_sel_ele = try_run(
                '''driver.find_element_by_class_name('content-container').find_element_by_xpath('//div/span/div/div[2]/div/div[3]/div/table/tbody')''')
            real_item = try_run('''find_element_by_xpath()''', component_sel_ele,
                                              '''//td/*[contains(text(),'{}')]'''.format(fusa_feature))
            try_run('''click()''', real_item)
            component_button_ele = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div[2]/div[2]/span[1]/button')''')
            try_run('''click()''', component_button_ele)

    # Select Test project
    if not (test_project == "null" or test_project == ""):
        test_project_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                       '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Test Project:')]/../../td[2]/div/div''')
        try_run('''click()''', test_project_element)
        test_project_list = test_project.split('++')
        test_project_option = ['Fusa', 'GP2.0', 'ISD', 'Open Source', 'Hybrid']
        for i in test_project_option:
            now_click_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                        '''//td[contains(text(),'{}')]'''.format(i))
            now_click_element_parent = try_run('''find_element_by_xpath()''', viewcontainer,
                                               '''//td[contains(text(),'{}')]/..'''.format(i))
            item_check_status = now_click_element_parent.get_attribute('aria-checked')
            if item_check_status == 'true':
                if i not in test_project_list:
                    try_run('''click()''', now_click_element)
            else:
                if i in test_project_list:
                    try_run('''click()''', now_click_element)
        try_run('''click()''', test_project_element)

    # Review Status
    if not (review_status == "null" or review_status == ""):
        review_status_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                        '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Review Status:')]/../../td[2]/div/div''')
        try_run('''click()''', review_status_element)
        review_status_list = review_status.split('++')
        review_status_option = ['Active Review Accepted', 'Active Review Rejected', 'Not Reviewed', 'Passive Review Accepted', 'Passive Review Rejected', 'QA Accepted', 'QA Rejected', 'Sample Review Accepted', 'Sample Review Rejected']
        for i in review_status_option:
            now_click_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                        '''//td[contains(text(),'{}')]'''.format(i))
            now_click_element_parent = try_run('''find_element_by_xpath()''', viewcontainer,
                                               '''//td[contains(text(),'{}')]/..'''.format(i))
            item_check_status = now_click_element_parent.get_attribute('aria-checked')
            if item_check_status == 'true':
                if i not in review_status_list:
                    try_run('''click()''', now_click_element)
            else:
                if i in review_status_list:
                    try_run('''click()''', now_click_element)   
        try_run('''click()''', review_status_element)

    # TC Complexity
    if not (tc_complexity == "null" or tc_complexity == ""):
        tc_complexity_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                        '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'TC  Complexity:')]/../../td[2]/div/select''')
        try_run('''Select().select_by_visible_text()''', tc_complexity_element, tc_complexity)

    # CPU Operation Mode
    if not (cpu_operation_mode == "null" or cpu_operation_mode == ""):
        cpu_operation_mode_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                     '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'CPU Operation Mode:')]/../../td[2]/div/select''')
        try_run('''Select().select_by_visible_text()''', cpu_operation_mode_element, cpu_operation_mode)
    
     # Test Level
    if not (Test_Level == "null" or Test_Level == ""):
        test_level_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                     '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Test Level:')]/../../td[2]/div/select''')
        try_run('''Select().select_by_visible_text()''', test_level_element, Test_Level)
    
    # input weight
    if not (weight == "null" or weight == ""):
        weight = weight.split(".0")[0]
        weight_element = try_run('''find_element_by_xpath()''', viewcontainer,
                                 '''//div/div/div/div/table/tbody/tr/td/label[contains(text(),'Weight:')]/../../td[2]/div/div[2]/input''')
        try_run('''clear()''', weight_element)
        try_run('''send_keys()''', weight_element, weight)

def _test_case_design(test_case_design, action='add'):
    if not (test_case_design == "null" or test_case_design == ""):
        #Switch to test case disign page
        test_case_design_page = try_run(
            '''driver.find_element_by_xpath("//li/a[@title='Test Case Design']")''')
        try_run('''click()''', test_case_design_page)
        time.sleep(5)
        if action == 'add':
            # click Add content
            editor_content_element = try_run('''driver.find_element_by_class_name('editor')''')
            add_content_element = try_run('''find_element_by_xpath()''', editor_content_element,
                                          '''//a[contains(text(),'Add Content')]''')
            try_run('''click()''', add_content_element)
            time.sleep(5)
        else:
            try:
                driver.find_element_by_class_name('editor').find_element_by_xpath('//a[contains(text(),"Add Content")]').click()
            except:
                driver.find_element_by_xpath('//div[2]/div/div/div/div/div/div[2]/span/a').click()
        # Input content in table area
        text_content = test_case_design.split('Table_value_area:<br/>')
        if len(text_content) == 1:
            text_content = test_case_design.split('Table_value_area:')
        content = '%s' % text_content[0].replace("<br/>", "\n")
        table_data = get_table_data(text_content[1])
        # js = 'document.getElementsByClassName("cke_wysiwyg_frame")[0].contentWindow.document.body.innerHTML="%s"' % content
        # driver.execute_script(js)
        body_frame = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'T')]/../../..//iframe")''')
        driver.switch_to_frame(body_frame)
        driver.find_element_by_tag_name('body').clear()
        action_chains = ActionChains(driver)
        action_chains.send_keys(content).perform()
        driver.switch_to_default_content()
        if text_content[1] != "":
            insert_btn = try_run('''driver.find_element_by_xpath('//div/span/span[2]/span[4]/span[3]')''')
            try_run('''click()''', insert_btn)
            edit_frame = try_run('''driver.find_element_by_class_name("cke_panel_frame")''')
            driver.switch_to_frame(edit_frame)
            table_btn = try_run('''driver.find_element_by_xpath('//div/div/span/a/span/span[2]')''')
            try_run('''click()''', table_btn)
            driver.switch_to_default_content()
            input_row = try_run('''driver.find_element_by_xpath('//tr/td[1]/div/div/div/input')''')
            try_run('''clear()''', input_row)
            tr_num = len(table_data)
            try_run('''send_keys()''', input_row, str(tr_num))
            input_td = try_run('''driver.find_element_by_xpath('//tr/td[2]/div/div/div/input')''')
            try_run('''clear()''', input_td)
            td_num = len(table_data[0])
            try_run('''send_keys()''', input_td, str(td_num))
            ok_btn = try_run('''driver.find_element_by_xpath('//tr/td/a/span')''')
            try_run('''click()''', ok_btn)
            table_frame = try_run('''driver.find_element_by_class_name("cke_wysiwyg_frame")''')
            driver.switch_to_frame(table_frame)
            my_table = try_run('''driver.find_element_by_xpath("//tbody")''')
            trs = my_table.find_elements_by_tag_name("tr")
            for i in range(tr_num):
                tds = trs[i].find_elements_by_tag_name("td")
                for j in range(td_num):
                    tds[j].click()
                    action_chains = ActionChains(driver)
                    action_chains.send_keys(table_data[i][j]).perform()
            driver.switch_to_default_content()

def _test_case_Formal_Review(formal_review, action='add'):
    if 'null' not in formal_review and formal_review != "":
        review_owner_list = formal_review.split('++')
        formal_review_page = try_run(
            '''driver.find_element_by_xpath("//li/a[@title='Formal Review']")''')
        try_run('''click()''', formal_review_page)
        time.sleep(5)
        if action == 'add':
            click_review = try_run('''driver.find_element_by_class_name('com-ibm-asq-common-web-ui-collapsible-section-content-outer-container').find_element_by_xpath('//div/div[2]/div/div/div/table/tbody/tr/td/div/span/a/span[2]')''')
            try_run('''click()''', click_review)
            reviewer = try_run('''driver.find_element_by_xpath("//div/table/tbody/tr/td/div/div/table/tbody/tr[@title='New Review']")''')
            try_run('''click()''', reviewer)
        for i in review_owner_list:
            add_review = try_run(
                '''driver.find_element_by_class_name('apporval-table-titlePanel').find_element_by_xpath('//div/div[3]/div/div/div/div/a')''')
            try_run('''click()''', add_review)
            time.sleep(2)
            review_input = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div/div[3]/input')''')
            try_run('''send_keys()''', review_input, i)
            time.sleep(3)
            se_reviewer = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div/div[5]/select')''')
            try_run('''Select().select_by_index()''', se_reviewer, 0)
            c_review_button = try_run(
                '''driver.find_element_by_class_name('content').find_element_by_xpath('//div/div/div[7]/button[1]')''')
            try_run('''click()''', c_review_button)

def _test_case_Requirement_Links(requirement_links, action='add'):
    if 'null' not in requirement_links and requirement_links != "":
        links_list = requirement_links.split('++')
        #Switch to Requirement Links Page
        requirement_links_page = try_run(
            '''driver.find_element_by_xpath("//li/a[@title='Requirement Links']")''')
        try_run('''click()''', requirement_links_page)
        time.sleep(5)

        for i in links_list:
            #click insert button
            insert_button = try_run('''driver.find_element_by_xpath('//table/tbody/tr[3]/td[3]/div/span[2]/a')''')
            try_run('''click()''', insert_button)

            #Select_SSG-OTC Item
            select_element = try_run('''driver.find_element_by_xpath('//*[@id="selectServiceProvider"]')''')
            try_run('''Select().select_by_visible_text()''', select_element, "SSG-OTC Product Management - DNG [rtc.intel.com]")

            # Click OK Button
            ok_button = try_run('''driver.find_element_by_xpath('//div/div[2]/div[2]/div[3]/div/div[2]/div[2]/span[1]/button')''')
            try_run('''click()''', ok_button)

            time.sleep(10)
            driver.switch_to.frame('https://rtc.intel.com/rqm0002001/web/dojo/resources/blank.html')

            if is_visible('//*[@id="jazz_ui_FilterBox_0"]/div/input', 60):
                # Select One Hypervisor
                ele = try_run('''driver.find_element_by_xpath('//div/table[1]/tbody/tr[1]/td/span/select')''')
                Select(ele).select_by_visible_text("One Hypervisor")
                time.sleep(10)

                #input id
                input_area = try_run('''driver.find_element_by_xpath('//div[@id="com_ibm_rdm_web_picker_ArtifactPickerPane_0"]/table[2]//tr[1]/td/div/div[3]/div[2]/div/div/input')''')
                try_run('''send_keys()''', input_area, i)
                try_run('''send_keys()''', input_area, Keys.ENTER)
                time.sleep(3)

                #Select 1st item
                first_link = try_run('''driver.find_element_by_xpath('//div/div[2]/a[1]/span[1]')''')
                try_run('''click()''', first_link)

                #click OK button
                ok_button = try_run(
                    '''driver.find_element_by_xpath('//*[@id="com_ibm_rdm_web_picker_RRCPicker_0"]/table/tbody/tr/td/div/button[1]')''')
                try_run('''click()''', ok_button)

            driver.switch_to.default_content()

def _test_case_Test_Scripts(test_scripts, action='add'):
    if 'null' not in test_scripts and test_scripts != "":
        test_scripts_list = test_scripts.split('++')
        # Switch to Test Scripts Page
        test_scripts_page = try_run(
            '''driver.find_element_by_xpath("//li/a[@title='Test Scripts']")''')
        try_run('''click()''', test_scripts_page)
        time.sleep(5)

        # click add button
        add_button = try_run('''driver.find_element_by_xpath('//table/tbody/tr[3]/td[3]/div/span[2]/a[@title="Add Test Scripts"]')''')
        try_run('''click()''', add_button)

        for i in test_scripts_list:
            # input id
            input_area = try_run(
                '''driver.find_element_by_xpath('//table/tbody/tr/td/div[2]/div[2]/table[1]/tbody/tr[1]/td/span/input')''')
            try_run('''clear()''', input_area)
            try_run('''send_keys()''', input_area, i)

            # Select 1st item
            first_checkbox = try_run(
                '''driver.find_element_by_xpath('//table/tbody/tr[1]/td[1]/span/div/input')''')
            try_run('''click()''', first_checkbox)

            # click Add button
            add_button = try_run(
                '''driver.find_element_by_xpath('//div/div[2]/div[2]/div[3]/div[1]/div[2]/div[2]/span[1]/button')''')
            try_run('''click()''', add_button)

        # Click close button
        close_button = try_run(
            '''driver.find_element_by_xpath('//div/div[2]/div[2]/div[3]/div[1]/div[2]/div[2]/span[3]/button')''')
        try_run('''click()''', close_button)

def _test_case_pre_condition(pre_conditon, action='add'):
    if 'null' not in pre_conditon and pre_conditon != "":
        # Switch to Pre Condition page
        pre_condition_page = try_run(
            '''driver.find_element_by_xpath("//li/a[@title='Pre-Condition']")''')
        try_run('''click()''', pre_condition_page)
        time.sleep(5)
        if action == 'add':
            # click Add content
            add_content_element = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'Pre-Condition')]/../../..//a[contains(text(),'Add Content')]")''')
            try_run('''click()''', add_content_element)
            time.sleep(5)
        else:
            try:
                driver.find_element_by_class_name('editor').find_element_by_xpath(
                    '//a[contains(text(),"Add Content")]').click()
            except:
                driver.find_element_by_xpath('//div[2]/div/div/div/div/div[2]/span/a').click()

        # input content
        body_frame = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'Pre-Condition')]/../../..//iframe")''')
        driver.switch_to_frame(body_frame)
        driver.find_element_by_tag_name('body').clear()
        action_chains = ActionChains(driver)
        action_chains.send_keys(pre_conditon).perform()
        driver.switch_to_default_content()

def _test_case_test_strategy_link(test_strategy_link, action='add'):
    if 'null' not in test_strategy_link and test_strategy_link != "":
        # Switch to Test Strategy page
        test_strategy_page = try_run(
            '''driver.find_element_by_xpath("//li/a[@title='Test Strategy Link']")''')
        try_run('''click()''', test_strategy_page)
        time.sleep(5)
        if action == 'add':
            # click Add content
            add_content_element = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'Test Strategy Link')]/../../..//a[contains(text(),'Add Content')]")''')
            try_run('''click()''', add_content_element)
            time.sleep(5)
        else:
            try:
                driver.find_element_by_class_name('editor').find_element_by_xpath(
                    '//a[contains(text(),"Add Content")]').click()
            except:
                driver.find_element_by_xpath('//div[2]/div/div/div/div/div[2]/span/a').click()

        # input content
        body_frame = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'Test Strategy Link')]/../../..//iframe")''')
        driver.switch_to_frame(body_frame)
        driver.find_element_by_tag_name('body').clear()
        action_chains = ActionChains(driver)
        action_chains.send_keys(test_strategy_link).perform()
        driver.switch_to_default_content()

def _test_case_test_description(test_description, action='add'):
    if 'null' not in test_description and test_description != "":
        # Switch to Test Description page
        test_description_page = try_run(
            '''driver.find_element_by_xpath("//li/a[@title='Test Description']")''')
        try_run('''click()''', test_description_page)
        time.sleep(5)
        if action == 'add':
            # click Add content
            add_content_element = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'Test Description')]/../../..//a[contains(text(),'Add Content')]")''')
            try_run('''click()''', add_content_element)
            time.sleep(5)
        else:
            try:
                driver.find_element_by_class_name('editor').find_element_by_xpath(
                    '//a[contains(text(),"Add Content")]').click()
            except:
                driver.find_element_by_xpath('//div[2]/div/div/div/div/div[2]/span/a').click()

        # input content
        body_frame = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'Test Description')]/../../..//iframe")''')
        driver.switch_to_frame(body_frame)
        driver.find_element_by_tag_name('body').clear()
        action_chains = ActionChains(driver)
        action_chains.send_keys(test_description).perform()
        driver.switch_to_default_content()

def _test_case_test_input(test_input, action='add'):
    if 'null' not in test_input and test_input != "":
        # Switch to Test Input page
        test_input_page = try_run(
            '''driver.find_element_by_xpath("//li/a[@title='Test Input']")''')
        try_run('''click()''', test_input_page)
        time.sleep(5)
        if action == 'add':
            # click Add content
            add_content_element = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'Test Input')]/../../..//a[contains(text(),'Add Content')]")''')
            try_run('''click()''', add_content_element)
            time.sleep(5)
        else:
            try:
                driver.find_element_by_class_name('editor').find_element_by_xpath(
                    '//a[contains(text(),"Add Content")]').click()
            except:
                driver.find_element_by_xpath('//div[2]/div/div/div/div/div[2]/span/a').click()

        # input content
        body_frame = try_run('''driver.find_element_by_xpath("//span[@name='Title'][contains(text(),'Test Input')]/../../..//iframe")''')
        driver.switch_to_frame(body_frame)
        driver.find_element_by_tag_name('body').clear()
        action_chains = ActionChains(driver)
        action_chains.send_keys(test_input).perform()
        driver.switch_to_default_content()

def add_test_case(title, case_template, owner, priority, description, component_value, test_group, test_category, execution_type, code_scan,
        test_platform, test_os, fusa_reference, fusa_feature, test_project, review_status, tc_complexity,
        cpu_operation_mode, weight, test_case_design, formal_review, requirement_links, test_scripts, first_action,
        save_action, pre_condition, Test_Level, test_strategy_link, test_description, test_input):

    driver.get("https://rtc.intel.com/rqm0002001/web/console/OTC%20Quality/_c5_6ksVxEeen7tuVCNHfog#action=com.ibm.rqm.planning.home.actionDispatcher&subAction=newTestCase&tq=1558405897000")
    try:
        _test_case_summary(title, case_template, owner, priority, description, component_value, test_group,
                              test_category,execution_type, code_scan,test_platform, test_os, fusa_reference,
                              fusa_feature, test_project,review_status, tc_complexity,cpu_operation_mode, weight, first_action, Test_Level, action='add')
        _test_case_design(test_case_design, action='add')
        _test_case_Formal_Review(formal_review, action='add')
        _test_case_Requirement_Links(requirement_links, action='add')
        _test_case_Test_Scripts(test_scripts, action='add')
        _test_case_pre_condition(pre_condition, action='add')
        if case_template == 'Fusa SRS Test Case Template':
            print(2222)
            _test_case_test_strategy_link(test_strategy_link, action='add')
            _test_case_test_description(test_description, action='add')
            _test_case_test_input(test_input, action='add')

        ###click save button
        js = "var q=document.documentElement.scrollTop=0"
        driver.execute_script(js)
        save_button = try_run('''driver.find_element_by_xpath('//*[@id="com_ibm_asq_common_web_ui_internal_view_layout_ViewHeaderLayout_0"]/div[1]/div/div[3]/div[2]/div/div[1]/button[2]')''')
        try_run('''click()''', save_button)
        js = "var q=document.documentElement.scrollTop=0"
        driver.execute_script(js)
        time.sleep(5)
        driver.execute_script(js)
        try:
            if not (save_action == "null" or save_action == ""):
                action_element = try_run('''driver.find_element_by_xpath("//table/tbody/tr[1]/td[4]/div/span[2]")''')
                try_run('''click()''', action_element)
                time.sleep(5)
                driver.execute_script(js)
                final_action = try_run('''driver.find_element_by_xpath('//span[contains(text(),"{}")]')'''.format(save_action))
                try_run('''click()''', final_action)
                time.sleep(5)
                try_run('''click()''', save_button)
        except Exception as e:
            logger.info('****************')
            logger.info(e)
            logger.info('****************')
            logger.info ("INFO:***need to change state manual***")
            return 1
        return 1
    except Exception as e:
        logger.info('****************')
        logger.info (e)
        logger.info('****************')
        return 0

def update_test_case(title, case_template, owner, priority, description, component_value, test_group, test_category, execution_type, code_scan,
        test_platform, test_os, fusa_reference, fusa_feature, test_project, review_status, tc_complexity,
        cpu_operation_mode, weight, test_case_design, formal_review, requirement_links, test_scripts, first_action,
        save_action, pre_condition, Search_String, Test_Level):
    driver.get("https://rtc.intel.com/rqm0002001/web/console/OTC%20Quality/_c5_6ksVxEeen7tuVCNHfog#action=com.ibm.rqm.planning.home.actionDispatcher&subAction=viewTestCases")
    time.sleep(20)
    try:
        # Locate the location of the search input box, clear and input the titile:case ID.
        if not (Search_String == "null" or Search_String == ""):
            print(Search_String)
            Search_element = try_run('''driver.find_element_by_xpath('//*[@id="com_ibm_asq_common_web_ui_internal_widgets_tableViewer_TableViewer_0"]/table[1]/tbody/tr[1]/td/span/input')''')
            try_run('''clear()''', Search_element)
            try_run('''send_keys()''', Search_element, Search_String)
        # Find the test project of title
        case_button = try_run('''driver.find_element_by_xpath('//div/div/table/tbody/tr')''')
        regexp = re.compile('[a-zA-Z]')
        if regexp.search(Search_String):
            case_button = try_run('''find_element_by_xpath()''', case_button, '//div[contains(text(),"{}")]'.format(Search_String))
        else:
            case_button = try_run('''find_element_by_xpath()''', case_button, '//td/a[contains(text(),"{}")]'.format(Search_String))
        try_run('''click()''', case_button)
        # excution update
        _test_case_summary(title, case_template, owner, priority, description, component_value, test_group,
                              test_category, execution_type, code_scan, test_platform, test_os, fusa_reference,
                              fusa_feature, test_project, review_status, tc_complexity, cpu_operation_mode, weight,
                              first_action, Test_Level, action='update')
        _test_case_design(test_case_design, action='update')
        _test_case_Formal_Review(formal_review, action='update')
        _test_case_Requirement_Links(requirement_links, action='update')
        _test_case_Test_Scripts(test_scripts, action='update')
        _test_case_pre_condition(pre_condition, action='update')
        # Click save button
        js = "var q=document.documentElement.scrollTop=0"
        driver.execute_script(js)
        save_button = try_run('''driver.find_element_by_xpath('//*[@id="com_ibm_asq_common_web_ui_internal_view_layout_ViewHeaderLayout_0"]/div[1]/div/div[3]/div[2]/div/div[1]/button[2]')''')
        try_run('''click()''', save_button)
        js = "var q=document.documentElement.scrollTop=0"
        driver.execute_script(js)
        try:
            if not (save_action == "null" or save_action == ""):
                action_element = try_run('''driver.find_element_by_xpath("//table/tbody/tr[1]/td[4]/div/span[2]")''')
                try_run('''click()''', action_element)
                time.sleep(3)
                final_action = try_run('''driver.find_element_by_xpath('//span[contains(text(),"{}")]')'''.format(save_action))
                try_run('''click()''', final_action)
                time.sleep(5)
                try_run('''click()''', save_button)
        except Exception as e:
            logger.info('****************')
            logger.info(e)
            logger.info('****************')
            logger.info ("INFO:***need to change test project of case manual***")
            return 1
        return 1
    except Exception as e:
        logger.info('****************')
        logger.info (e)
        logger.info('****************')
        return 0

def get_logger():
    logger = logging.getLogger("rqmlogger")
    logger.setLevel(logging.DEBUG)
    file_name = "RQM_" + time.strftime("%Y_%m_%d_%H_%M_%S") + ".log"
    fh = logging.FileHandler(file_name)
    fh.setLevel(logging.DEBUG)
    sh = logging.StreamHandler()
    sh.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(asctime)s - %(filename)s[line:%(lineno)d] - %(levelname)s: %(message)s")
    fh.setFormatter(formatter)
    sh.setFormatter(formatter)
    logger.addHandler(fh)
    return logger

if len(sys.argv)<3 and len(sys.argv)>1:
    print('''pls, give two parameters: operation(add/update) and xlsx fileame, like: python RQM.py update demo.xlsx True''')
    sys.exit(1)
else:
    xlsxfile = sys.argv[2]
    case_list = readxlsx.get_test_case_list(xlsxfile)
    logger = get_logger()
    n=1
    for t in case_list:
        Title = t.Title.decode('utf-8')
        Case_Template = t.Case_Template.decode('utf-8')
        Owner = t.Owner.decode('utf-8')
        Priority = t.Priority.decode('utf-8')
        Description = t.Description.decode('utf-8')
        Component_Value = t.Component_Value.decode('utf-8')
        Test_Group = t.Test_Group.decode('utf-8')
        Test_Category = t.Test_Category.decode('utf-8')
        Execution_Type = t.Execution_Type.decode('utf-8')
        Code_Scan = t.Code_Scan.decode('utf-8')
        Test_Platform = t.Test_Platform.decode('utf-8')
        Test_OS = t.Test_OS.decode('utf-8')
        Fusa_Reference = t.Fusa_Reference.decode('utf-8')
        Fusa_Feature = t.Fusa_Feature.decode('utf-8')
        Test_Project = t.Test_Project.decode('utf-8')
        Review_Status = t.Review_Status.decode('utf-8')
        TC_Complexity = t.TC_Complexity.decode('utf-8')
        CPU_Operation_Mode = t.CPU_Operation_Mode.decode('utf-8')
        Weight = t.Weight.decode('utf-8')
        Test_Case_Design = t.Test_Case_Design.decode('utf-8')
        Formal_Review = t.Formal_Review.decode('utf-8')
        Requirement_Links = t.Requirement_Links.decode('utf-8')
        Test_Scripts = t.Test_Scripts.decode('utf-8')
        First_Action = t.First_Action.decode('utf-8')
        Save_Action = t.Save_Action.decode('utf-8')
        Pre_Condition = t.Pre_Condition.decode('utf-8')
        Search_String = t.Search_String.decode('utf-8')
        Test_Level = t.Test_Level.decode('utf-8')
        Test_Strategy_Link = t.Test_Strategy_Link.decode('utf-8')
        Test_Description = t.Test_Description.decode('utf-8')
        Test_Input = t.Test_Input.decode('utf-8')
        # open chrome
        driver = webdriver.Chrome()
        driver.maximize_window()
        driver.implicitly_wait(2)

        Requirement_Links = Requirement_Links.split(".0")[0]
        Test_Scripts = Test_Scripts.split(".0")[0]
        Search_String = Search_String.split(".0")[0]
        print('*****{}*****'.format(Test_Case_Design))
        if sys.argv[1] == 'add':
            print("Begin to add the %d row case:***%s***" % (n,t.Title.decode('utf-8')))
            result = add_test_case(Title, Case_Template, Owner, Priority, Description, Component_Value, Test_Group, Test_Category,
            Execution_Type, Code_Scan, Test_Platform, Test_OS, Fusa_Reference, Fusa_Feature, Test_Project,
            Review_Status, TC_Complexity, CPU_Operation_Mode, Weight, Test_Case_Design, Formal_Review, Requirement_Links,
            Test_Scripts, First_Action, Save_Action, Pre_Condition, Test_Level, Test_Strategy_Link, Test_Description, Test_Input)
        elif sys.argv[1] == 'update':
            print("Begin to update the %d row case:***%s***" % (n,t.Title.decode('utf-8')))
            result = update_test_case(Title, Case_Template, Owner, Priority, Description, Component_Value, Test_Group, Test_Category,
            Execution_Type, Code_Scan, Test_Platform, Test_OS, Fusa_Reference, Fusa_Feature, Test_Project,
            Review_Status, TC_Complexity, CPU_Operation_Mode, Weight, Test_Case_Design, Formal_Review, Requirement_Links,
            Test_Scripts, First_Action, Save_Action, Pre_Condition, Search_String,Test_Level)
        else:
            print("Please check the first parameter: add or update.")
            sys.exit(1)
        print('result: {}'.format(result))
        if result == 1:
            logger.info("add the %d row case:***%s*** successfully!" % (n,t.Title.decode('utf-8')))
            driver.quit()
        else:
            logger.info("add the %d row case:***%s*** failed!" % (n, t.Title.decode('utf-8')))
            driver.quit()

