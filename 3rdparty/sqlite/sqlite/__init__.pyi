SQLITE_DONE: int = 101
SQLITE_ROW: int = 100
SQLITE_MISUSE: int = 21
SQLITE_OK: int = 0

class Stmt():
    """
    It represents a parsed SQL statement that can be executed against a database.
    It can only be created by calling the prepare method of a Database object.
    
    ### Format String Spec
    The format string is a string that specifies the types of the 
    parameters to be bound to the statement, and also the types of the columns 
    to be returned by the statement.
    
    It shall only contain the following characters:
    - 'b': blob, in python it is bytes.
    - 'd': double, in python it is float.
    - 'i': integer, in python it is int.
    - 'l': 64-bit integer, in python it is int.
    - 'n': null, in python it is None.
    - 's': string, in python it is str.
    """

    def bind(self, fmt: str, value: list | tuple) -> int:
        """
        Bind values to the parameters of the statement. The number of 
        values must match the number of parameters in the statement, 
        and the types of the values must match the types specified in the format string.
      
        :param fmt: A format string that specifies the types of the parameters to be bound.
        :param value: A list or tuple of values to be bound to the parameters.
        :return: SQLITE_OK if the values are successfully bound, or an error code otherwise.
        """
        ...


    def get_columns(self, fmt: str) -> tuple:
        """
        :param fmt: a format string the specifies types of the columns of one row.
      
        Example:
        >>> type(stmt)
        <class 'sqlite.Stmt'>
        >>> while stmt.step() == SQLITE_ROW:
        ...    row: tuple[int, str] = stmt.get_columns('is')
        ...
        >>> # All rows have been processed, and the statement is done.
        """
        ...


    def step(self) -> int:
        """
        This function must be called one or more times to evaluate the statement.
      
        :return: a SQLite status code. Possible values are:
        - SQLITE_ROW: a row is produced and to be got with `get_columns`.
        - SQLITE_DONE: the statement has finished executing and there are no more rows to be produced.
        - SQLITE_MISUSE: the statement is misused.
        """
        ...

    def __bool__(self) -> bool: ...


class Database():
    """
    Represents an opened SQLite database connection.
    """ 

    def close(self) -> None:
        """
        Close the database connection. After this method is called, the Database object
        is not usable.
        """
        ...


    def prepare(self, sql: str) -> Stmt:
        """
        To execute an SQL statement, it must first be compiled into a byte-code
        program using one of these routines. Or, in other words, this routine
        are constructors for the prepared `Stmt` object.
        """
        ...

    def __bool__(self) -> bool: ...


def open(filename: str, read_only: bool | None = False) -> Database:
    """
    Open a database file. If the file does not exist, it will be created.
    :param filename: The name of the database file to open.
    :param read_only: If True, the database will be opened in read-only mode. 
    :return: A Database object representing the opened database.
    """
    ...


def open_memory() -> Database:
    """
    Open an in-memory database. This database will not be 
    saved to disk and will be lost when the connection is closed.
    """
    ...
