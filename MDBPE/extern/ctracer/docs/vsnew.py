# -*- coding: utf-8 -*-
"""
    Pygment style loosely based on vs/vscode

    To install: 
     drop it into the styles subpackage of your Pygments distribution one style class per style, 
     where the file name is the style name and the class name is StylenameClass.
     (e.g. /home/<user>/.local/lib/python2.7/site-packages/pygments/styles)
"""

from pygments.style import Style
from pygments.token import Keyword, Name, Comment, String, Error, \
     Operator, Generic, Number, Literal


class VsnewStyle(Style):

    background_color = "#1e1e1e"
    default_style = ""

    styles = {
        Comment:                   "#57a64a",
        Comment.Preproc:           "#9b9b9b",
        Keyword:                   "#569cd6",
        Operator.Word:             "#0000ff",
        Keyword.Type:              "#569cd6",
        Name.Class:                "#4ec9b0",
        Name.Builtin:              "#569cd6",
        String:                    "#d69d85",
        Number:                    "#b5cea8",
        Literal:                   "#dcdcdc",

        Generic.Heading:           "bold",
        Generic.Subheading:        "bold",
        Generic.Emph:              "italic",
        Generic.Strong:            "bold",
        Generic.Prompt:            "bold",

        Error:                     "border:#FF0000"
    }