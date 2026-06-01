
def safe_index(lst: list, value) -> int | None:
    try:
        return lst.index(value)
    except ValueError:
        return None
