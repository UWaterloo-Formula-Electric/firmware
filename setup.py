import setuptools

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setuptools.setup(
    name="wfe-beaglebone-app", # Replace with your own username
    version="0.0.1",
    author="Waterloo Formula Electric",
    author_email="djgood@uwaterloo.ca",
    description="All Python code that gets run on the Beaglebone",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/pypa/sampleproject",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
    data_files=[('data', 'common-all/Data/2018CAR.dbc')],
)
