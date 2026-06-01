from skbuild import setup

if __name__ == "__main__":
    # ENABLE_INSTALL is turned off because
    # header files and the static library are not needed for the Python bindings.
    setup(
        name = 'pyptr',
        packages = ['pyptr'],
        version = '0.0.1',
        description = 'Simple weakref extension',
        license = 'MIT',
    )

