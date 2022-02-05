import argparse
from pathlib import Path
import typing
import clang.cindex
import json

COLPROC_BASE_CLASSES = (
    'ColProc', 'Variable<T>'
)


def parse_args():
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('-i', help='Input class directory to analyze')
    parser.add_argument('-o', help='Class info json output')
    parser.add_argument('--include_path', nargs='+', default=[], help='Include directories path compile options')
    parser.add_argument('--root_class', nargs='+', help='Root class names to search derived classes for')

    return parser.parse_args()



def filter_node_list_by_node_kind(
    nodes: typing.Iterable[clang.cindex.Cursor],
    kinds: list
) -> typing.Iterable[clang.cindex.Cursor]:
    result = []

    for i in nodes:
        if i.kind in kinds:
            result.append(i)

    return result



def class_get_base_class_names(
    c_class: clang.cindex.Cursor,
) -> typing.Iterable[str]:
    c_base_classes = filter_node_list_by_node_kind(
        c_class.get_children(), 
        [clang.cindex.CursorKind.CXX_BASE_SPECIFIER]
    )

    return [base.displayname.split()[-1] for base in c_base_classes]



def filter_class_node_by_base_class(
    nodes: typing.Iterable[clang.cindex.Cursor],
    base_classes: typing.Iterable[str]
) -> typing.Iterable[clang.cindex.Cursor]:
    result = []
    for c in nodes:    
        c_base_classes = class_get_base_class_names(c)
        for base in c_base_classes:
            if base in base_classes:
                result.append(c)
                break

    return result



def filter_class_node_by_base_class_tree(
    nodes: typing.Iterable[clang.cindex.Cursor],
    base_class_names: typing.Iterable[str]
) -> typing.Iterable[clang.cindex.Cursor]:
    result = []

    while True:
        child_classes = filter_class_node_by_base_class(
            nodes, base_class_names
        )

        if len(child_classes) == 0:
            break

        result = result + child_classes
        base_class_names = list( (c.displayname for c in child_classes) )

    return result



def filter_class_unique_names(
    nodes: typing.Iterable[clang.cindex.Cursor]
) -> typing.Iterable[clang.cindex.Cursor]:
    class_names = set()
    result = []

    for c in nodes:
        if c.displayname not in class_names:
            class_names.add(c.displayname)
            result.append(c)

    return result



def filter_class_non_abstract_class(
    nodes: typing.Iterable[clang.cindex.Cursor]
) -> typing.Iterable[clang.cindex.Cursor]:
    result = []
    for c in nodes: 
        if c.is_abstract_record() == False:
            result.append(c)

    return result        



def find_colproc_classes(directory, include_path='') -> typing.Iterable[clang.cindex.Cursor]:
    result = []
    ccargs    = '-x c++ --std=c++11'.split() + [f'-I{path}' for path in include_path]
    for header in Path(directory).rglob('*.h'):
        index = clang.cindex.Index.create()
        translation_unit = index.parse(
            header,
            args=ccargs, 
            options=
                clang.cindex.TranslationUnit.PARSE_INCOMPLETE |
                clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES |
                0x4000  #https://clang.llvm.org/doxygen/group__CINDEX__TRANSLATION__UNIT.html#ggab1e4965c1ebe8e41d71e90203a723fe9a5493ee3e0dc0ee1ba0896fbf43938552
        )

        root_node = list(translation_unit.cursor.get_children())

        classes = filter_node_list_by_node_kind(
            root_node, 
            [clang.cindex.CursorKind.CLASS_DECL, clang.cindex.CursorKind.CLASS_TEMPLATE, clang.cindex.CursorKind.STRUCT_DECL]
        )

        colproc_variable_classes = filter_class_node_by_base_class_tree (
            classes,
            COLPROC_BASE_CLASSES
        )

        result = result + colproc_variable_classes

    return filter_class_unique_names(result)


###################################################################
# STAGE 1: Parse colproc sources to find class to be serializable #
###################################################################
args = parse_args()
colproc_classes = filter_class_non_abstract_class(
    find_colproc_classes(args.i, args.include_path)
)

###############################################################
# STAGE 2: Save class information for code generation utility #
###############################################################
def class_info_dict(
    c_class: clang.cindex.Cursor,
    root_class: str=''
) -> dict:
    """
    Output:
    {
        "className": "GeneratorRainbow",
        "rootClass": "ColProc",
        "baseClass": ["Generator"],
        "constructors": [
            {
                "arguments": (
                    {
                        "type": "Variable<uint32_t>*",
                        "name": "length"
                    },
                    {
                        "type": "Variable<uint32_t>*",
                        "name": "angle"
                    },
                    {
                        "type": "Variable<uint32_t>*",
                        "name": "cycle_period_ms"
                    }
                )
            }
        ]
    }
    """
    
    c_constructors = (
        c for c in filter_node_list_by_node_kind(
            c_class.get_children(), [clang.cindex.CursorKind.CONSTRUCTOR]
        )
    )

    constructors = []
    for c in c_constructors:
        args = []
        for param in c.get_arguments():
            name = param.displayname
            type = ' '.join(t.spelling for t in param.get_tokens() if t.spelling != name)
            args.append({
                'type': type,
                'name': name
            })
        
        constructors.append({'arguments': args})

    return {
        'className': c_class.displayname,
        'rootClass': root_class,
        'baseClass': list(class_get_base_class_names(c_class)),
        'constructors': constructors
    }


class_info_json = json.dumps( 
    tuple(class_info_dict(c) for c in colproc_classes),
    indent=2
)
print(class_info_json)