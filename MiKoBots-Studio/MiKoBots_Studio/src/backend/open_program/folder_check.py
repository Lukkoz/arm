import os
import json
import shutil
from backend.file_managment import get_file_path, get_path_folder
import zipfile
import sys
import platform

# This build of MiKoBots Studio is made for one robot: it is the only robot in the library.
ROBOT = "ARM"


def bundled_data_zip():
    """The robot data shipped with the app (assets/MiKoBots_data/MiKoBots.zip)."""
    if getattr(sys, 'frozen', False):
        base_path = os.path.dirname(sys.executable)
        if platform.system() == "Darwin":
            base_path = os.path.join(os.path.dirname(base_path), 'Resources')
    else:
        # running from source: src/backend/open_program -> MiKoBots_Studio
        base_path = os.path.join(os.path.dirname(__file__), '..', '..', '..')
    return os.path.normpath(os.path.join(base_path, 'assets', 'MiKoBots_data', 'MiKoBots.zip'))


def EnsureOnlyRobot():
    """Makes ROBOT the only robot, also when the data folder already existed before this app
    was installed: adds ROBOT from the shipped data if it is missing, moves every other robot
    to Robot_library_disabled (nothing is deleted) and selects ROBOT."""
    library = str(get_file_path("/Robot_library"))
    os.makedirs(library, exist_ok=True)

    if not os.path.isdir(os.path.join(library, ROBOT)):
        prefix = f"MiKoBots/Robot_library/{ROBOT}/"
        with zipfile.ZipFile(bundled_data_zip(), 'r') as zip_ref:
            for name in zip_ref.namelist():
                if name.startswith(prefix):
                    zip_ref.extract(name, get_path_folder())
        print(f"Robot {ROBOT} added to the robot library")

    disabled = str(get_file_path("/Robot_library_disabled"))
    for entry in os.listdir(library):
        if entry == ROBOT or not os.path.isdir(os.path.join(library, entry)):
            continue
        os.makedirs(disabled, exist_ok=True)
        target = os.path.join(disabled, entry)
        n = 1
        while os.path.exists(target):
            target = os.path.join(disabled, f"{entry}_{n}")
            n += 1
        shutil.move(os.path.join(library, entry), target)
        print(f"Robot {entry} moved to {target}")

    # settings.json item 4 is the selected robot (by name)
    settings_path = str(get_file_path("/settings/settings.json"))
    with open(settings_path, 'r') as file:
        settings = json.load(file)
    if isinstance(settings, list) and len(settings) > 4 and settings[4] != ROBOT:
        settings[4] = ROBOT
        with open(settings_path, 'w') as file:
            json.dump(settings, file, indent=4)


def FolderCheck():
    file_path = get_file_path("")

    if not os.path.isdir(file_path):
        # first start: unpack the shipped data
        file_path = get_path_folder()
        os.makedirs(file_path, exist_ok=True)

        with zipfile.ZipFile(bundled_data_zip(), 'r') as zip_ref:
            zip_ref.extractall(file_path)
        
    # check if all the folders are present 
    
    
    
    
    
    
    ## settings folder
    folder_path = get_file_path("/settings")
    if not (os.path.exists(folder_path) and os.path.isfile(folder_path)):
        os.makedirs(folder_path, exist_ok=True)
        
    file_path = get_file_path("/settings/settings.json")
    if not (os.path.exists(file_path) and os.path.isfile(file_path)):
        setting_file = ["0", "0", "0", "0", "0"]
        with open(file_path, 'w') as file:
            json.dump(setting_file, file, indent=4)
                    
    # simulation folder
    folder_path = get_file_path("/Simulation_library")
    if not (os.path.exists(folder_path) and os.path.isfile(folder_path)):
        os.makedirs(folder_path, exist_ok=True)
    
    file_path = get_file_path("/Simulation_library/settings.json")
    if not (os.path.exists(file_path) and os.path.isfile(file_path)):
        setting_file = []
        with open(file_path, 'w') as file:
            json.dump(setting_file, file, indent=4)
            
    # robot folder
    folder_path = get_file_path("/Robot_library")
    if not (os.path.exists(folder_path) and os.path.isfile(folder_path)):
        os.makedirs(folder_path, exist_ok=True)

    try:
        EnsureOnlyRobot()
    except Exception as e:
        print(f"Error: could not set up the robot {ROBOT}: {e}")


    ## get language files