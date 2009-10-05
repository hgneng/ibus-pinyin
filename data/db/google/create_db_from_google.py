#!/usr/bin/env python
import sqlite3
from pydict import *
from id import *
import sys

def get_sheng_yun(pinyin):
    if pinyin == None:
        return None, None
    if pinyin == "ng":
        return "", "en"
    for i in xrange(2, 0, -1):
        t = pinyin[:i]
        if t in SHENGMU_DICT:
            return t, pinyin[len(t):]
    return "", pinyin

def create_db():
    # con = sqlite3.connect("main.db")
    # con.execute ("PRAGMA synchronous = NORMAL;")
    # con.execute ("PRAGMA temp_store = MEMORY;")
    # con.execute ("PRAGMA default_cache_size = 5000;")
    print "PRAGMA synchronous = NORMAL;"
    print "PRAGMA temp_store = MEMORY;"
    print "PRAGMA default_cache_size = 5000;"


    sql = "CREATE TABLE py_phrase_%d (phrase TEXT, freq INTEGER, %s);"
    for i in range(0, 16):
        column = []
        for j in range(0, i + 1):
            column.append ("s%d INTEGER" % j)
            column.append ("y%d INTEGER" % j)
        print sql % (i, ",".join(column))
        # con.execute(sql % (i, column))
        # con.commit()

    validate_hanzi = get_validate_hanzi()
    records = list(read_phrases(validate_hanzi))
    records.sort(lambda a, b: 1 if a[1] > b[1] else -1)
    records_new = []
    i = 0
    max_freq = 0.0
    for hanzi, freq, pinyin in records:
        if max_freq / freq <  1 - 0.001:
            max_freq = freq
            i = i + 1
        records_new.append((hanzi, i, pinyin))
    records_new.reverse()
    
    print "BEGIN;"
    insert_sql = "INSERT INTO py_phrase_%d VALUES (%s);"
    for hanzi, freq, pinyin in records_new:
        columns = []
        for py in pinyin:
            s, y = get_sheng_yun(py)
            s, y = pinyin_id[s], pinyin_id[y]
            columns.append(s)
            columns.append(y)
        values = "'%s', %d, %s" % (hanzi.encode("utf8"), freq, ",".join(map(str,columns)))
            
        sql = insert_sql % (len(hanzi) - 1, values)
        print sql
    print "COMMIT;"
    print "VACUUM;"


def get_validate_hanzi():
    validate_hanzi = file("valid_utf16.txt").read().decode("utf16")
    return set(validate_hanzi)

def read_phrases(validate_hanzi):
    buf = file("rawdict_utf16_65105_freq.txt").read()
    buf = unicode(buf, "utf16")
    buf = buf.strip()
    for l in buf.split(u'\n'):
        hanzi, freq, flag, pinyin = l.split(u' ', 3)
        freq = float(freq)
        pinyin = pinyin.split()
        if any(map(lambda c: c not in validate_hanzi, hanzi)):
            continue
        yield hanzi, freq, pinyin

def main():
    create_db()
 
if __name__ == "__main__":
    main()
