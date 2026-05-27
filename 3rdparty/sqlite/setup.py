from skbuild import setup

if __name__ == "__main__":
    setup(
        name = 'sqlite',
        packages = ['sqlite'],
        version = '3.53.1',
        description = 'SQLite is a C library that implements a SQL database engine.',
        license = 'Public Domain',
        cmake_args = [
            "-DSQLITE_BUILD_PYTHON=ON",
            "-DSQLITE_ENABLE_TESTS=OFF",
        ],
    )
