import ctypes
import sys
import configparser
from CBFSController import CheckFileExist, AddName, AddID, Controller

""" Entry point into the program """
def main(argv):

    # Connects to a config file to read variables from
    config = configparser.ConfigParser()
    config.read("config.ini")

    # Get all the processes (by names and IDs) from the config file
    for name in config['PROCESS-NAMES']:
        AddName(config['PROCESS-NAMES'][name])

    for id in config['PROCESS-IDS']:
        AddID(int(config['PROCESS-IDS'][id]))

    # Check to see if the file exists
    file_exists = CheckFileExist(config['CBFS']['directory'])
    if not file_exists:
        # Install
        print("File does not exist. Need to install.")

    # Begins the controller for the program
    Controller(config['CBFS']['key'])


if __name__ == "__main__":
    main(sys.argv)