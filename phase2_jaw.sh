path = "src/Model.cpp"
with open(path, "rb") as f: src = f.read()

old = (
    b"\t\t\t// Every frame: just draw \xe2\x80\x94 no data upload.\n"
    b"\t\t\tvbos[i].drawElements(GL_TRIANGLES, meshes[i].getNumIndices());\n"
)
new = (
    b"\t\t\t// Every frame: just draw \xe2\x80\x94 no data upload.\n"
    b"\t\t\tif (i == 1) {\n"
    b"\t\t\t\tofPushMatrix();\n"
    b"\t\t\t\tofTranslate(0, -70, 0);\n"
    b"\t\t\t\tofRotateXDeg(jawAngle);\n"
    b"\t\t\t\tofTranslate(0, 70, 0);\n"
    b"\t\t\t\tvbos[i].drawElements(GL_TRIANGLES, meshes[i].getNumIndices());\n"
    b"\t\t\t\tofPopMatrix();\n"
    b"\t\t\t} else {\n"
    b"\t\t\t\tvbos[i].drawElements(GL_TRIANGLES, meshes[i].getNumIndices());\n"
    b"\t\t\t}\n"
)
assert old in src, "ERROR: VBO draw line not found"
src = src.replace(old, new, 1)

with open(path, "wb") as f: f.write(src)
print("Done — verify: sed -n '75,95p' src/Model.cpp")
