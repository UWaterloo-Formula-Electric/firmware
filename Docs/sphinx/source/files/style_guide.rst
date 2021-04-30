#########################
Documentation Style Guide
#########################

This document focuses on style-guide.
It is a kind of coding standards applied to documentation files.
It is not about documentation content.


***************************************
RestructuredText with Sphinx directives
***************************************

This documentation uses Python-sphinx, which itself uses reStructuredText
syntax.


*********
Filenames
*********

Use only lowercase alphanumeric characters and ``_`` (underscore) symbol.

Suffix filenames with the ``.rst`` extension.

***********
Whitespaces
***********

Indentation
===========

Indent with 2 spaces.

Except:

* ``toctree`` directive requires a 3 spaces indentation.

Blank lines
===========

Two blank lines before overlined sections, i.e. before H1 and H2.
One blank line before other sections.
See `Headings`_ for an example.

One blank line to separate directives.

.. code-block:: rst

  Some text before.

  .. note::

    Some note.

Exception: directives can be written without blank lines if they are only one
line long.

.. code-block:: rst

  .. note:: A short note.


***********
Line length
***********

Limit all lines to a maximum of 79 characters.


********
Headings
********

Use the following symbols to create headings:

#. ``#`` with overline
#. ``*`` with overline
#. ``=``
#. ``-``
#. ``^``
#. ``"``

As an example:

.. code-block:: rst

  ##################
  H1: document title
  ##################

  Introduction text.


  *********
  Sample H2
  *********

  Sample content.


  **********
  Another H2
  **********

  Sample H3
  =========

  Sample H4
  ---------

  Sample H5
  ^^^^^^^^^

  Sample H6
  """""""""

  And some text.

If you need more than heading level 4 (i.e. H5 or H6), then you should consider
creating a new document.

There should be only one H1 in a document.

.. note::

  See also `Sphinx's documentation about sections`_.


***********
Code blocks
***********

Use the ``code-block`` directive **and** specify the programming language. As
an example:

.. code-block:: rst

  .. code-block:: python

    import this


********************
Links and references
********************

Use links and references footnotes with the ``target-notes`` directive.
As an example:

.. code-block:: rst

  #############
  Some document
  #############

  Some text which includes links to `Example website`_ and many other links.

  `Example website`_ can be referenced multiple times.

  (... document content...)

  And at the end of the document...

  **********
  References
  **********

  .. target-notes::

  .. _`Example website`: http://www.example.com/

