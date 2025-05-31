import os
import platform
import utils
import subprocess
from pathlib import Path

class PermakeConfiguration:
    premake_version = "5.0.0-beta6"
    permake_license_url = "https://raw.githubusercontent.com/premake/premake-core/master/LICENSE.txt"
    premake_directory = os.path.abspath("scripts/premake")

    if platform.system() == "Windows":
        premake_archive_urls = f'https://github.com/premake/premake-core/releases/download/v{premake_version}/premake-{premake_version}-windows.zip'
    elif platform.system() == "Linux":
        premake_archive_urls = f"https://github.com/premake/premake-core/releases/download/v{premake_version}/premake-{premake_version}-linux.tar.gz"

    @classmethod
    def validate(self):
        if not self.is_premake_installed():
            print("Premake is not installed. Please install it first.")
            return False
        return True
    
    @classmethod
    def is_premake_installed(self):
        if platform.system() == "Windows":
            premake_exe = Path(f'{self.premake_directory}/premake5.exe')
        elif platform.system() == "Linux":
            premake_exe = Path(f'{self.premake_directory}/premake5')
        
        if not premake_exe.exists():
            print(f"Premake executable not found at {premake_exe}. Please ensure it is installed correctly.")
            return self.install_premake()
        
        return True
    
    @classmethod
    def install_premake(self):
        permission_granted = False

        while not permission_granted:
            reply = str(input("Premake is not installed. Do you want to install it? (y/n): ")).strip().lower()
            if reply in ['y', 'yes']:
                permission_granted = True
                print("Installing Premake...")
            elif reply in ['n', 'no']:
                print("Premake installation cancelled.")
                return False
            
        if platform.system() == "Windows":
            premake_path = f'{self.premake_directory}/premake-{self.premake_version}-windows.zip'
        elif platform.system() == "Linux":
            premake_path = f'{self.premake_directory}/premake-{self.premake_version}-linux.tar.gz'

        if not os.path.exists(premake_path):
            print("Downloading premake5 to {0:s}".format(premake_path))
            utils.download_file(self.premake_archive_urls, premake_path)

            premake_license_path = f'{self.premake_directory}/LICENSE.txt'
            print("Downloading Premake license to {0:s}".format(premake_license_path))
            utils.download_file(self.permake_license_url, premake_license_path)

        print(f"Extracting Premake: {premake_path} to {self.premake_directory}")
        utils.extract_archive(premake_path, delete_after_extraction=True)
        
        return True


if __name__ == "__main__":
    premake_config = PermakeConfiguration()

    print("Setting up Premake...")
    if not premake_config.validate():
        print("Premake validation failed. Exiting setup.")
        exit(1)

    print("Premake is installed and validated successfully.")

    premake_path = f"{premake_config.premake_directory}/premake5"
    arguments = ["vs2022", "--file=premake5.lua"] if platform.system() == "Windows" else ["gmake2", "--file=premake5.lua"]

    # Execute premake5 with arguments
    subprocess.run([premake_path] + arguments)
