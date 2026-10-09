# -*- coding: utf-8 -*-
"""
作业二数据准备脚本：把点名册 xls 导出成 C++ 头文件 rosterdata.h

用法：
    python tools/export_roster.py

输入：
    samp4_13TableWidget/周一点名册.xls
    samp4_13TableWidget/周四点名册.xls
输出：
    samp4_13TableWidget/samp4_13TableWidget/rosterdata.h

xls 里只有 序号/学号/姓名/班级 四列；
性别、院系、专业、修读性质、籍贯 由下面的 DEFAULT_* 与 OVERRIDE 补齐。
"""
import os
import glob
import xlrd

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
XLS_DIR = os.path.join(ROOT, "samp4_13TableWidget")
OUT_H = os.path.join(XLS_DIR, "samp4_13TableWidget", "rosterdata.h")

# 全班统一缺省值
DEFAULT_SEX = "男"
DEFAULT_DEPT = "计算机科学与技术学院"
DEFAULT_MAJOR = "软件工程"
DEFAULT_NATURE = "初修"
DEFAULT_HOMETOWN = ""

# 逐人补充数据（键 = 学号）
OVERRIDE = {
    "2024414300105": {"sex": "男", "hometown": "广东佛山"},
    "2024414300106": {"sex": "男", "hometown": "广东广州"},
    "2024414300107": {"sex": "女", "hometown": "四川成都"},
    "2024414300110": {"sex": "男", "hometown": "广东深圳"},
    "2024414300111": {"sex": "男", "hometown": "广东东莞"},
}


def read_roster(path):
    """读取一份点名册，返回 [(学号, 姓名, 班级), ...]"""
    sheet = xlrd.open_workbook(path).sheet_by_index(0)
    rows = []
    for r in range(5, sheet.nrows):          # 前 5 行为表头
        sid = str(sheet.cell_value(r, 1)).strip()
        name = str(sheet.cell_value(r, 2)).strip()
        cls = str(sheet.cell_value(r, 3)).strip()
        if sid.endswith(".0"):
            sid = sid[:-2]
        if not sid and not name:
            continue
        rows.append((sid, name, cls))
    return rows


def c_str(text):
    """转义成 C++ 字符串字面量（UTF-8 源码）"""
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def emit_array(name, rows):
    lines = ["static const RosterRow %s[] = {" % name]
    for sid, sname, cls in rows:
        extra = OVERRIDE.get(sid, {})
        sex = extra.get("sex", DEFAULT_SEX)
        dept = extra.get("dept", DEFAULT_DEPT)
        major = extra.get("major", DEFAULT_MAJOR)
        nature = extra.get("nature", DEFAULT_NATURE)
        home = extra.get("hometown", DEFAULT_HOMETOWN)
        fields = [sid, sname, cls, sex, dept, major, nature, home]
        lines.append("    { %s }," % ", ".join(c_str(f) for f in fields))
    lines.append("};")
    lines.append("static const int %sCount = %d;" % (name, len(rows)))
    return "\n".join(lines)


def main():
    monday_path = os.path.join(XLS_DIR, "周一点名册.xls")
    thursday_path = os.path.join(XLS_DIR, "周四点名册.xls")
    monday = read_roster(monday_path)
    thursday = read_roster(thursday_path)

    header = """// 自动生成，请勿手工编辑：tools/export_roster.py
// 数据来源：周一点名册.xls、周四点名册.xls + 手工补充字段
#ifndef ROSTERDATA_H
#define ROSTERDATA_H

struct RosterRow {
    const char *id;        // 学号
    const char *name;      // 姓名
    const char *cls;       // 行政班级
    const char *sex;       // 性别
    const char *dept;      // 院(系)/部
    const char *major;     // 专业
    const char *nature;    // 修读性质
    const char *hometown;  // 籍贯
};

%s

%s

#endif // ROSTERDATA_H
""" % (emit_array("kMondayRoster", monday),
       emit_array("kThursdayRoster", thursday))

    with open(OUT_H, "w", encoding="utf-8", newline="\n") as f:
        f.write(header)

    print("written:", OUT_H)
    print("monday=%d thursday=%d" % (len(monday), len(thursday)))


if __name__ == "__main__":
    main()
