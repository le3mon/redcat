import xml.etree.ElementTree as ET


def pretty_print(current, parent=None, index=-1, depth=0):
        for i, node in enumerate(current):
            pretty_print(node, current, i, depth + 1)
            # print(i, node)
        if parent is not None:
            if index == 0:
                parent.text = '\n' + ('  ' * depth + "")
                current.text = " {} ".format(current.text)
            else:
                current.text = " {} ".format(current.text)
                parent[index - 1].tail = '\n' + ('  ' * depth + "")
            if index == len(parent) - 1:
                current.tail = '\n' + ('  ' * (depth - 1)+ "")   


# root = ET.Element("classVarDec")
# ele1 = ET.Element("test")
# ele2 = ET.Element("parameterList")
# root.append(ele1)
# root.append(ele2)
# # ele1.append(ele2)

# ele1.text = "test"
# ele2.text = "app"


# # ele1.remove(sub_el)

# pretty_print(root)

# tree = ET.ElementTree(root)

# with open("test.xml", "wb") as file:
#     tree.write(file, encoding="utf-8", short_empty_elements=False)


file = "./test/test_out.xml"
tree = ET.parse(file)
pretty_print(tree.getroot())
ET.dump(tree)