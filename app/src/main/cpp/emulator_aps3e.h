// SPDX-License-Identifier: WTFPL

#ifndef APS3E_EMULATOR_APS3E_H
#define APS3E_EMULATOR_APS3E_H

#include "Utilities/File.h"
#include "Loader/iso.h"

namespace ae{
    extern std::string game_id;
    extern bool install_firmware(int fd);
    extern bool install_pkg(iso_fs& iso_fs, const std::string& path);
    extern bool install_pkg(const char* path);
    extern bool install_pkg(int pkg_fd);
    extern bool allow_eboot_decrypt(const fs::file& eboot_file);
    extern bool install_edat(const fs::file& edat_file);

    extern std::string get_gpu_info();
    extern std::string get_cpu_info();

}
#endif //APS3E_EMULATOR_APS3E_H
