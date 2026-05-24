import sqlite

SQLITE_OK: int = 0
SQLITE_DONE: int = 101
SQLITE_ROW: int = 100

def test_db():
    db = sqlite.open_memory()
    res = db.prepare("CREATE TABLE test (id INTEGER , name TEXT)").step()
    assert res == SQLITE_DONE

    for row in [(1, '😀'), (2, '😭')]:
        stmt = db.prepare("INSERT INTO test VALUES (?, ?);")
        res = stmt.bind('is', row)
        assert res == SQLITE_OK
        assert stmt.step() == SQLITE_DONE
        del stmt

    stmt = db.prepare("SELECT name, id FROM test ORDER BY id DESC;")
    results: list[tuple[str, int]] = []
    while stmt.step() == SQLITE_ROW:
        res = stmt.get_columns('si')
        print(res)
        results.append(res)

    assert results == [('😭', 2), ('😀', 1)]
    db.close()


if __name__ == "__main__":
    test_db()
