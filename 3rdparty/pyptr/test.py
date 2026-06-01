import pyptr

def test_pointer():
    num = 0xc0ffee
    pt = pyptr.PyPointer(num)

    for _ in range(10):
        assert pt() is num


def test_refcnt():
    num = 0xc0ffee

    refcnt = pyptr.refcnt(num)
    pointers = []

    for _ in range(10):
        pointers.append(pyptr.PyPointer(num))

    # the pointer does not borrow references.
    # and we do not use '==' because of variations of reference counting.
    assert abs(pyptr.refcnt(num) - refcnt) < 3

    del pointers
    assert abs(pyptr.refcnt(num) - refcnt) < 3

