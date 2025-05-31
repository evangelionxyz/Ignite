import sys
import os
import requests
import time
import urllib
import subprocess
import json
import tarfile
from zipfile import ZipFile
import platform

if platform.system() == "Windows":
    import winreg

def set_env_variable(variable_name, directory_path):
    if platform.system() == "Windows":
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment", 0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(key, variable_name, 0, winreg.REG_EXPAND_SZ, directory_path)
        winreg.CloseKey(key)
    elif platform.system() == "Linux":
        os.environ[variable_name] = directory_path
        with open('/etc/environment', 'a') as file:
            file.write(f'\n{variable_name}="{directory_path}"\n')


def get_env_variable(name):
    if platform.system() == "Windows":
        key = winreg.CreateKey(winreg.HKEY_LOCAL_MACHINE, r"System\CurrentControlSet\Control\Session Manage\Environment")
        try:
            return winreg.QueryValueEx(key, name)[0]
        except:
            return None
    elif platform.system() == "Linux":
        value = os.environ.get(name)
        if value is not None:
            return True
        try:
            with open('/etc/environment', 'r') as file:
                for line in file:
                    if line.startswith(f'{name}='):
                        return line.split('=', 1)[1].strip().strip('"')
        except FileNotFoundError:
            return None
    return None


def get_user_env_variable(name):
    if platform.system() == "Windows":
        key = winreg.CreateKey(winreg.HKEY_CURRENT_USER, r"Environment")
        try:
            return winreg.QueryValueEx(key, name)[0]
        except:
            return None
    elif platform.system() == "Linux":
        return os.environ.get(name)
    return None


def set_system_path_variable(new_path):
    if platform.system() == "Windows":
        current_path = current_path = os.environ.get('PATH', '')
        if new_path not in current_path:
            new_path = f'{current_path};{new_path}'
            set_env_variable("Path", new_path)
            return True
    if platform.system() == "Linux":
        current_path = os.environ.get('PATH', '')
        if new_path not in current_path.split(':'):
            new_path = f'{current_path}:{new_path}'
            os.environ['PATH'] = new_path
            with open('/etc/environment', 'a') as file:
                file.write(f'\nPATH="{new_path}"\n')
            return True
    return False


def download_file(url, filepath):
    path = filepath
    filepath = os.path.abspath(filepath)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)

    if (type(url) is list):
        for url_option in url:
            print("Downloading", url_option)

            try:
                download_file(url_option, filepath)
                return
            except urllib.error.URLError as e:
                print(f"URL Error encountered: {e.reason}. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
            except urllib.error.HTTPError as e:
                print(f"HTTP Error  encountered: {e.code}. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
            except:
                print(f"Something went wrong. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
        raise ValueError(f"Failed to download {filepath}")
    
    if (not(type(url) is str)):
        raise ValueError(f"Argument 'url' must be type of list or string")
    
    with open(filepath, 'wb') as f:
        headers = {'User-Agent': "Mozilla/5.0 (Macintosh Intel Mac Os X 10_15_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.97 Safari/537.36"}
        response = requests.get(url, headers = headers, stream = True)
        total = response.headers.get('content-length')

        if total is None:
            f.write(response.content)
        else:
            downloaded = 0
            total = int(total)
            startTime = time.time()

            # Download filechunk
            for data in response.iter_content(chunk_size = max(int(total / 1000), 1024 * 1024)):
                downloaded += len(data)
                f.write(data)

                # Print progress
                done = int(50 * downloaded / total)
                percentage = (downloaded / total) * 100
                elpased_time = time.time() - startTime
                avg_kb_per_sec = downloaded / 1024 / elpased_time

                if avg_kb_per_sec > 1024:
                    avg_speed_string = '{:.2f} MB/s'.format(avg_kb_per_sec / 1024)
                else:
                    avg_speed_string = '{:.2f} MB/s'.format(avg_kb_per_sec)

                sys.stdout.write('\r[{}{}] {:2f}% ({})    '.format(
                    '█' * done, ' ' * (50 - done), percentage, avg_speed_string
                ))
                sys.stdout.flush()

            sys.stdout.write('\n')
            sys.stdout.flush()


def extract_archive(filepath, delete_after_extraction=True):
    arch_filepath = os.path.abspath(filepath)
    arch_file_location = os.path.dirname(arch_filepath)

    if platform.system() == "Windows":
        zip_file = dict()
        zip_file_content_size = 0

        with ZipFile(arch_filepath, 'r') as zip_file_folder:
            for name in zip_file_folder.namelist():
                zip_file[name] = zip_file_folder.getinfo(name).file_size

            zip_file_content_size = sum(zip_file.values())
            extracted_content_size = 0
            
            start_time = time.time()
            for zipped_filename, zipped_file_size in zip_file.items():
                unzipped_filepath = os.path.abspath(f"{arch_file_location}/{zipped_filename}")
                os.makedirs(os.path.dirname(unzipped_filepath), exist_ok=True)

                if not os.path.isfile(unzipped_filepath):
                    zip_file_folder.extract(zipped_filename, path = arch_file_location, pwd = None)
                    extracted_content_size += zipped_file_size
                else:
                    extracted_content_size += zipped_file_size

                if zip_file_content_size > 0:
                    done = int(50 * extracted_content_size / zip_file_content_size)
                    percentage = extracted_content_size / zip_file_content_size * 100
                else:
                    done = 50
                    percentage = 100

                elapsed_time = time.time() - start_time

                try:
                    avg_kb_per_sec = extracted_content_size / 1024 / elapsed_time
                except ZeroDivisionError:
                    avg_kb_per_sec = 0

                if avg_kb_per_sec > 1024:
                    avg_speed_string = '{:.2f} MB/s'.format(avg_kb_per_sec / 1024)
                else:
                    avg_speed_string = '{:.2f} KB/s'.format(avg_kb_per_sec)

                sys.stdout.write('\r[{}{}] {:2f}% ({})'
                                .format('█' * done, ' ' * (50 - done), percentage, avg_speed_string))
                
                sys.stdout.flush()

        sys.stdout.write('\n')

    elif platform.system() == "Linux":
        file = tarfile.open(arch_filepath)
        file.extractall(arch_file_location)
        file.close()

    if delete_after_extraction:
        os.remove(arch_filepath)
        print(f"Deleted archive file: {arch_filepath}")