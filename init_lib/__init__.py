import importlib
import glob


class TestMain:
    pass


# load seperated test classes

for part in glob.glob("init_lib/test_*.py"):
    filename = part.split("/")[-1].split(".")[0]
    module = importlib.import_module(f"init_lib.{filename}")
    assert hasattr(module, "TestSeperated"), f"TestSeperated class not found in {part}"
    seperated_class = getattr(module, "TestSeperated")
    for name in dir(seperated_class):
        if not name.startswith("_"):
            setattr(TestMain, name, getattr(seperated_class, name))
