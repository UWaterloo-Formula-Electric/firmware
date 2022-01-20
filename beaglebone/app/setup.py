import setuptools

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setuptools.setup(
    name="wfe",
    version="0.0.1",
    author="Waterloo Formula Electric",
    author_email="djgood@uwaterloo.ca",
    description="All Python code that gets run on the Beaglebone",
    long_description=long_description,
    long_description_content_type="text/markdown",
    packages=setuptools.find_packages(),
    package_data={'wfe': 
        ['../../../../common/Data/2018CAR.dbc',
         'json/*.json']
    },
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
    install_requires=[
        "python-can",
        "cantools",
        "PySide2",
        "pyzmq"
    ],
    entry_points={
        "console_scripts": [
            'wfe-dashboard=wfe.dashboard:main',
            'wfe-monitor=wfe.can_monitor:main',
            'wfe-database=wfe.database.database:main',
            'wfe-simulator=wfe.can_simulator:main'
        ]
    }
)
