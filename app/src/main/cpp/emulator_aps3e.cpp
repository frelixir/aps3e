// SPDX-License-Identifier: WTFPL
#include <android/log.h>
#include <jni.h>
#include <filesystem>
#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>
#include "emulator_aps3e.h"
#include "cpuinfo.h"
#include "Emu/system_config.h"
#include "Loader/PSF.h"


#define LOG_TAG "aps3e_native"

#define LOGW(...) {      \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,"%s : %d",__func__,__LINE__);\
	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG,__VA_ARGS__);\
}

#define LOGE(...) {      \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,"%s : %d",__func__,__LINE__);\
	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG,__VA_ARGS__);\
}

static jboolean j_install_firmware(JNIEnv* env,jobject self,jint pup_fd){
    //jboolean is_copy=false;
    //const char* path=env->GetStringUTFChars(pup_path,&is_copy);
    jboolean result= ae::install_firmware(pup_fd);
    //env->ReleaseStringUTFChars(pup_path,path);
    return result;
}

/*
static jboolean j_install_pkg(JNIEnv* env,jobject self,jstring pkg_path){
	jboolean is_copy=false;
	const char* path=env->GetStringUTFChars(pkg_path,&is_copy);
    jboolean result= aps3e_util::install_pkg(path);
    env->ReleaseStringUTFChars(pkg_path,path);
	return result;
}*/

static jboolean j_install_pkg(JNIEnv* env,jobject self,jint pkg_fd){
    //jboolean is_copy=false;
    //const char* path=env->GetStringUTFChars(pkg_path,&is_copy);
    jboolean result= ae::install_pkg(pkg_fd);
    //env->ReleaseStringUTFChars(pkg_path,path);
    return result;
}

static jboolean j_install_edat(JNIEnv* env,jobject self,jint edat_fd){
    fs::file edat_f=fs::file::from_fd(edat_fd);
    return ae::install_edat(edat_f);
}

static jobject j_meta_info_from_dir(JNIEnv* env,jobject self,jstring jdir_path){

    auto fetch_psf_path=[](const std::string& dir_path){
        std::string sub_paths[]={
                "/PARAM.SFO","/PS3_GAME/PARAM.SFO"
        };
        for(std::string& sub_path:sub_paths){
            std::string psf_path=dir_path+sub_path;
            if(std::filesystem::exists(psf_path))
                return psf_path;
        }
        return std::string{};
    };

    auto fetch_eboot_path=[](const std::string& dir_path){
        std::string sub_paths[]=
                {
                        "/EBOOT.BIN",
                        "/USRDIR/EBOOT.BIN",
                        "/USRDIR/ISO.BIN.EDAT",
                        "/PS3_GAME/USRDIR/EBOOT.BIN",
                };
        for(std::string& sub_path:sub_paths){
            std::string eboot_path=dir_path+sub_path;
            if(std::filesystem::exists(eboot_path))
                return eboot_path;
        }
        return std::string{};
    };

    jboolean is_copy=false;
    const char* _dir_path=env->GetStringUTFChars(jdir_path,&is_copy);
    const std::string dir_path=_dir_path;
    env->ReleaseStringUTFChars(jdir_path, _dir_path);

    psf::registry psf=psf::load_object(fetch_psf_path(dir_path));
#if 1
    for(const auto& [key,value]:psf){
        switch (value.type()) {
            case psf::format::array:

                LOGW("key %s is array!",key.c_str());
                break;
            case psf::format::string:
                LOGW("key %s svalue %s",key.c_str(),value.as_string().c_str());
                break;
            case psf::format::integer:
                LOGW("key %s ivalue %d",key.c_str(),value.as_integer());
                break;
        }
    }
#endif
    jclass cls_MetaInfo=env->FindClass("aenu/aps3e/Emulator$MetaInfo");
    jmethodID mid_MetaInfo_ctor=env->GetMethodID(cls_MetaInfo,"<init>","()V");
    jfieldID fid_MetaInfo_eboot_path=env->GetFieldID(cls_MetaInfo,"eboot_path","Ljava/lang/String;");
    jfieldID fid_MetaInfo_name=env->GetFieldID(cls_MetaInfo,"name","Ljava/lang/String;");
    jfieldID fid_MetaInfo_serial=env->GetFieldID(cls_MetaInfo,"serial","Ljava/lang/String;");
    jfieldID fid_MetaInfo_category=env->GetFieldID(cls_MetaInfo,"category","Ljava/lang/String;");
    jfieldID fid_MetaInfo_version=env->GetFieldID(cls_MetaInfo,"version","Ljava/lang/String;");
    jfieldID fid_MetaInfo_decrypt=env->GetFieldID(cls_MetaInfo,"decrypt","Z");
    jfieldID  fid_MetaInfo_resolution=env->GetFieldID(cls_MetaInfo,"resolution","I");
    jfieldID  fid_MetaInfo_sound_format=env->GetFieldID(cls_MetaInfo,"sound_format","I");

    jobject meta_info=env->NewObject(cls_MetaInfo,mid_MetaInfo_ctor);

    std::string v;
    env->SetObjectField(meta_info,fid_MetaInfo_eboot_path,env->NewStringUTF((v=fetch_eboot_path(dir_path)).c_str()));
    env->SetBooleanField(meta_info,fid_MetaInfo_decrypt,ae::allow_eboot_decrypt(fs::file(v)));
    env->SetObjectField(meta_info,fid_MetaInfo_name,env->NewStringUTF((v=psf::get_string(psf,"TITLE","")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_serial,env->NewStringUTF((v=psf::get_string(psf,"TITLE_ID","")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_category,env->NewStringUTF((v=psf::get_string(psf,"CATEGORY","??")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_version,env->NewStringUTF((v=psf::get_string(psf,"APP_VER","")).c_str()));
    env->SetIntField(meta_info,fid_MetaInfo_resolution,psf::get_integer(psf,"RESOLUTION",0));
    env->SetIntField(meta_info,fid_MetaInfo_sound_format,psf::get_integer(psf,"SOUND_FORMAT",0));
    return meta_info;
}


static jobject j_meta_info_from_iso(JNIEnv* env,jobject self,jint fd,jstring jiso_uri_path){

    jclass cls_MetaInfo=env->FindClass("aenu/aps3e/Emulator$MetaInfo");
    jmethodID mid_MetaInfo_ctor=env->GetMethodID(cls_MetaInfo,"<init>","()V");
    jfieldID fid_MetaInfo_iso_uri=env->GetFieldID(cls_MetaInfo,"iso_uri","Ljava/lang/String;");
    jfieldID fid_MetaInfo_name=env->GetFieldID(cls_MetaInfo,"name","Ljava/lang/String;");
    jfieldID fid_MetaInfo_serial=env->GetFieldID(cls_MetaInfo,"serial","Ljava/lang/String;");
    jfieldID fid_MetaInfo_category=env->GetFieldID(cls_MetaInfo,"category","Ljava/lang/String;");
    jfieldID fid_MetaInfo_version=env->GetFieldID(cls_MetaInfo,"version","Ljava/lang/String;");
    jfieldID fid_MetaInfo_icon=env->GetFieldID(cls_MetaInfo,"icon","[B");
    jfieldID fid_MetaInfo_decrypt=env->GetFieldID(cls_MetaInfo,"decrypt","Z");
    jfieldID  fid_MetaInfo_resolution=env->GetFieldID(cls_MetaInfo,"resolution","I");
    jfieldID  fid_MetaInfo_sound_format=env->GetFieldID(cls_MetaInfo,"sound_format","I");

    std::unique_ptr<iso_fs> iso=iso_fs::from_fd(fd);
    if(!iso->load()) {
        LOGW("Failed to load iso");
        return NULL;
    }

    if(!iso->exists(":PS3_GAME/USRDIR/EBOOT.BIN")) {
        LOGW("EBOOT.BIN not found");
        return NULL;
    }

    std::vector<uint8_t> psf_data=iso->get_data_tiny(":PS3_GAME/PARAM.SFO");
    if(psf_data.empty()) {
        LOGW("Failed to load PARAM.SFO");
        return NULL;
    }

    psf::registry psf=psf::load_object(fs::file(psf_data.data(),psf_data.size()),"PS3_GAME/PARAM.SFO"sv);

    jobject meta_info=env->NewObject(cls_MetaInfo,mid_MetaInfo_ctor);
    env->SetObjectField(meta_info,fid_MetaInfo_iso_uri,jiso_uri_path);
    std::string v;
    env->SetObjectField(meta_info,fid_MetaInfo_name,env->NewStringUTF((v=psf::get_string(psf,"TITLE","")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_serial,env->NewStringUTF((v=psf::get_string(psf,"TITLE_ID","")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_category,env->NewStringUTF((v=psf::get_string(psf,"CATEGORY","??")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_version,env->NewStringUTF((v=psf::get_string(psf,"APP_VER","")).c_str()));
    env->SetIntField(meta_info,fid_MetaInfo_resolution,psf::get_integer(psf,"RESOLUTION",0));
    env->SetIntField(meta_info,fid_MetaInfo_sound_format,psf::get_integer(psf,"SOUND_FORMAT",0));

    std::vector<uint8_t> icon_data=iso->get_data_tiny(":PS3_GAME/ICON0.PNG");
    if(!icon_data.empty()) {
        jbyteArray icon_array=env->NewByteArray(icon_data.size());
        env->SetByteArrayRegion(icon_array,0,icon_data.size(),reinterpret_cast<const jbyte*>(icon_data.data()));
        env->SetObjectField(meta_info,fid_MetaInfo_icon,icon_array);
    }

    env->SetBooleanField(meta_info,fid_MetaInfo_decrypt,ae::allow_eboot_decrypt(fs::file(*iso,":PS3_GAME/USRDIR/EBOOT.BIN")));

    return meta_info;
}
static void j_setup_game_id(JNIEnv* env,jobject self,jstring id){
    const char* _id=env->GetStringUTFChars(id,NULL);
    ae::game_id=_id;
    env->ReleaseStringUTFChars(id,_id);
}
/*public native int get_cpu_core_count();
public native String get_cpu_name(int core_idx);
public native int get_cpu_max_mhz(int core_idx);*/
static jint j_get_cpu_core_count(JNIEnv* env,jobject self){
    return cpu_get_core_count();
}
static jstring j_get_cpu_name(JNIEnv* env,jobject self,jint core_idx){
    const std::string& name=cpu_get_processor_name(core_idx);
    jstring r= env->NewStringUTF(name.c_str());
    return r;
}
static jint j_get_cpu_max_mhz(JNIEnv* env,jobject self,jint core_idx){
    return cpu_get_max_mhz(core_idx);
}

static jstring j_simple_device_info(JNIEnv* env, jobject thiz)
{

    std::string vv;//=get_auther_info();

    vv+=ae::get_cpu_info();
    vv+="\n"+ae::get_gpu_info();

    return env->NewStringUTF(vv.c_str());
}

static auto gen_key=[](const std::string& name)->std::string{
    std::string k=name;
    if(size_t p=k.find("(");p!=std::string::npos){
        k=k.substr(0,p);
    }

    //replace(' ','_');
    //replace('/','_')
    for(auto& c:k){
        if(c==' '){
            c='_';
        }
        else if(c=='/'){
            c='_';
        }
        else if(c=='|'){
            c='_';
        }

    }

    //replace("-","");
    size_t p=k.find("-");
    while(p!=std::string::npos){
        k.erase(p,1);
        p=k.find("-");
    }

    //移除尾部的_
    while (k.at(k.size()-1)=='_'){
        k.erase(k.size()-1,1);
    }

    std::transform(k.begin(),k.end(),k.begin(),[](char c){
        return std::tolower(c);
    });

    return k;
};

static const std::string gen_skips[]={
        "Core|SPU LLVM Lower Bound",
        "Core|SPU LLVM Upper Bound",
        "Audio|Audio Device",
        "Audio|Microphone Devices",
        "Input/Output|Camera ID",
        "Input/Output|Emulated Midi devices",
        "System|System Name",
        "System|PSID high",
        "System|PSID low",
        "System|HDD Model Name",
        "System|HDD Serial Number",
        "Miscellaneous|GDB Server",
        "Miscellaneous|Window Title Format",

        "Video|Force Convert Texture"

        "Video|Performance Overlay|Font",
        "Video|Performance Overlay|Body Color (hex)",
        "Video|Performance Overlay|Body Background (hex)",
        "Video|Performance Overlay|Title Color (hex)",
        "Video|Performance Overlay|Title Background (hex)",
};

static bool gen_is_parent(const std::string& parent_name){
    if(parent_name=="Core"||parent_name=="Video"||parent_name=="Audio"||parent_name=="Input/Output"
       ||parent_name=="System"||parent_name=="Savestate"||parent_name=="Miscellaneous")
        return true;
    return false;
}

#define SEEKBAR_PREF_TAG "aenu.preference.SeekBarPreference"
#define CHECKBOX_PREF_TAG "aenu.preference.CheckBoxPreference"
#define LIST_PREF_TAG "aenu.preference.ListPreference"

static jstring generate_config_xml(JNIEnv* env,jobject self){

    auto gen_one_preference=[&](const std::string parent_name,cfg::_base* node)->std::string{
        std::stringstream out;

        std::string parent_name_l=gen_key(parent_name);

        const std::string& name=node->get_name();
        const std::string key=gen_key(name);

        switch (node->get_type()) {
            case cfg::type::_bool:
                out<<"<" CHECKBOX_PREF_TAG " app:title=\"@string/emulator_settings_"<<parent_name_l<<"_"<<key<<"\" \n";
                out<<"app:key=\""<<parent_name<<"|"<<name<<"\" />\n";
                break;
            case cfg::type::_int:
            case cfg::type::uint:
                out<<"<" SEEKBAR_PREF_TAG " app:title=\"@string/emulator_settings_"<<parent_name_l<<"_"<<key<<"\" \n";
                out<<"app:min=\""<<node->get_min()<<"\"\n";
                if(node->get_max()!=-1)
                    out<<"android:max=\""<<node->get_max()<<"\"\n";
                else
                    out<<"android:max=\"0x7fffffff\"\n";
                out<<"app:showSeekBarValue=\"true\"\n";
                out<<"app:key=\""<<parent_name<<"|"<<name<<"\" />\n";
                break;

            case cfg::type::_enum:
                out<<"<" LIST_PREF_TAG " app:title=\"@string/emulator_settings_"<<parent_name_l<<"_"<<key<<"\" \n";
                out<<"app:entries=\""<<"@array/"<<parent_name_l<<"_"<<key<<"_entries\"\n";
                out<<"app:entryValues=\""<<"@array/"<<parent_name_l<<"_"<<key<<"_values\"\n";
                out<<"app:key=\""<<parent_name<<"|"<<name<<"\" />\n";
                break;
            default:

                out<<"<PreferenceScreen app:title=\"@string/emulator_settings_"<<parent_name_l<<"_"<<key<<"\"\n";
                out<<"app:key=\""<<parent_name<<"|"<<name<<"\" />\n";
                break;
        }
        return out.str();
    };

    auto gen_vk_debug_preferences=[&](const std::string parent_name,cfg::_base* node)->std::string {
        std::stringstream out;

        std::string parent_name_l=gen_key(parent_name);
        const std::string &name = node->get_name();
        const std::string key = gen_key(name);

        if(node->get_type()==cfg::type::node){
            out<<"<PreferenceScreen app:title=\"@string/emulator_settings_"<<parent_name_l<<"_"<<key<<"\" \n";
            out<<"app:key=\""<<parent_name<<"|"<<name<<"\" >\n";
            std::string node_key=parent_name+"|"+name;
            for(auto n3:reinterpret_cast<cfg::node*>(node)->get_nodes()) {
                if(n3->get_type()!=cfg::type::_bool){
                    LOGE("unexpected type");
                    continue;
                }
                out<<"<" CHECKBOX_PREF_TAG " app:title=\""<<n3->get_name()<<"\" \n";
                out<<"app:key=\""<<node_key<<"|"<<n3->get_name()<<"\" />\n";
            }
            out<<"</PreferenceScreen>\n";
        }

        return out.str();
    };

    std::ostringstream out;
    out<<R"(
<?xml version="1.0" encoding="utf-8"?>
<PreferenceScreen
    xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:app="http://schemas.android.com/apk/res-auto">
    )";

    for(auto n:g_cfg.get_nodes()){
        const std::string& name=n->get_name();
        if(gen_is_parent(name)){
            out<<" <PreferenceScreen app:title=\"@string/emulator_settings_"<<gen_key(name)<<"\" \n";
            out<<"app:key=\""<<name<<"\" >\n";

            for(auto n2:reinterpret_cast<cfg::node*>(n)->get_nodes()){

                if(std::find(std::begin(gen_skips),std::end(gen_skips),name+"|"+n2->get_name())!=std::end(gen_skips))
                    continue;

                if(n2->get_type()==cfg::type::node){
                    out<<"<PreferenceScreen app:title=\"@string/emulator_settings_"<<gen_key(name)<<"_"<<gen_key(n2->get_name())<<"\" \n";
                    out<<"app:key=\""<<name+"|"+n2->get_name()<<"\" >\n";
                    if((name+"|"+n2->get_name())=="Core|Thread Affinity Mask"){
                        out<<"</PreferenceScreen>\n";
                        continue;
                    }
                    for(auto n3:reinterpret_cast<cfg::node*>(n2)->get_nodes()) {
                        if(std::find(std::begin(gen_skips),std::end(gen_skips),name+"|"+n2->get_name()+"|"+n3->get_name())!=std::end(gen_skips))
                            continue;
                        //
                        if(n3->get_type()==cfg::type::node){
                            out << "\n" <<gen_vk_debug_preferences(name+"|"+n2->get_name(),n3)<< "\n";
                            continue;
                        }

                        out << "\n" << gen_one_preference(name+"|"+n2->get_name(), n3) << "\n";
                    }
                    out<<"</PreferenceScreen>\n";
                }
                else{
                    out<<"\n"<< gen_one_preference(name,n2)<<"\n";
                }
            }
            out<<"</PreferenceScreen>\n";
        }
    }

    out<<"</PreferenceScreen>\n";

    return env->NewStringUTF(out.str().c_str());
}
#undef SEEKBAR_PREF_TAG
#undef CHECKBOX_PREF_TAG
#undef LIST_PREF_TAG

//public native String generate_strings_xml();
static jstring generate_strings_xml(JNIEnv* env,jobject self){

    auto gen_one_string=[&](const std::string parent_name,cfg::_base* node)->std::string{
        std::stringstream out;

        std::string parent_name_l=gen_key(parent_name);

        const std::string& name=node->get_name();
        const std::string key=gen_key(name);

        out<<"<string name=\"emulator_settings_"<<parent_name_l<<"_"<<key<<"\">"<<name<<"</string>\n";
        if(node->get_type()==cfg::type::_enum){
            out<<"<string-array name=\""<<parent_name_l<<"_"<<key<<"_entries\">\n";
            std::vector<std::string> list=node->to_list();
            for(const auto& e:list){
                out<<"<item>"<<e<<"</item>\n";
            }
            out<<"</string-array>\n";
        }

        return out.str();
    };

    std::ostringstream out;

    for(auto n:g_cfg.get_nodes()){
        const std::string& name=n->get_name();
        if(gen_is_parent(name)){

            out<<"<string name=\"emulator_settings_"<<gen_key(name)<<"\">"<<name<<"</string>\n";

            for(auto n2:reinterpret_cast<cfg::node*>(n)->get_nodes()){

                //Video下的3个子项
                if(n2->get_type()==cfg::type::node){

                    out<<"<string name=\"emulator_settings_"<<gen_key(name+"|"+n2->get_name())<<"\">"<<n2->get_name()<<"</string>\n";
                    for(auto n3:reinterpret_cast<cfg::node*>(n2)->get_nodes()) {
                        out << gen_one_string(name+"|"+n2->get_name(), n3);
                    }
                }
                else{
                    out<< gen_one_string(name,n2);
                }
            }
        }
    }

    //array
    for(auto n:g_cfg.get_nodes()){
        const std::string& name=n->get_name();
        if(gen_is_parent(name)){

            for(auto n2:reinterpret_cast<cfg::node*>(n)->get_nodes()){

                switch (n2->get_type()) {
                    case cfg::type::_enum: {

                        out << "<string-array name=\"" << gen_key(name + "|" + n2->get_name())
                            << "_values\">\n";
                        std::vector<std::string> list = n2->to_list();
                        for (const auto &e: list) {
                            out << "<item>" << e << "</item>\n";
                        }
                        out << "</string-array>\n";
                        break;
                    }
                        //Video下的3个子项
                    case cfg::type::node:{
                        for(auto n3:reinterpret_cast<cfg::node*>(n2)->get_nodes()) {
                            out << "<string-array name=\""<<gen_key(name+"|"+n2->get_name()+"|"+n3->get_name())<<"_values\">\n";
                            std::vector<std::string> list=n3->to_list();
                            for(const auto& e:list){
                                out<<"<item>"<<e<<"</item>\n";
                            }
                            out<<"</string-array>\n";
                        }
                        break;
                    }
                    default:
                        break;
                }

            }
        }
    }

    return env->NewStringUTF(out.str().c_str());
}

static jstring generate_java_string_arr(JNIEnv* env,jobject self){

    auto gen_one_key_string=[&](const std::string parent_name,cfg::_base* node,cfg::type test_ty)->std::string{
        std::string r="";
        if(node->get_type()==test_ty){
            return r+"\""+parent_name+"|"+node->get_name()+"\",\n";
        }
        return r;
    };

    auto gen_one_key_array=[&](const std::string prefix,cfg::type test_ty)->std::string{

        std::ostringstream out;
        out<<prefix<<"\n";
        for(auto n:g_cfg.get_nodes()){
            const std::string& name=n->get_name();
            if(gen_is_parent(name)){

                for(auto n2:reinterpret_cast<cfg::node*>(n)->get_nodes()){

                    if(std::find(std::begin(gen_skips),std::end(gen_skips),name+"|"+n2->get_name())!=std::end(gen_skips))
                        continue;


                    if(n2->get_type()!=cfg::type::node) {
                        out<< gen_one_key_string(name,n2,test_ty);
                        continue;
                    }

                    //node
                    for(auto n3:reinterpret_cast<cfg::node*>(n2)->get_nodes()) {
                        if(std::find(std::begin(gen_skips),std::end(gen_skips),name+"|"+n2->get_name()+"|"+n3->get_name())!=std::end(gen_skips))
                            continue;
                        if(n3->get_type()!=cfg::type::node){
                            out << gen_one_key_string(name+"|"+n2->get_name(), n3,test_ty);
                            continue;
                        }

                        for(auto n4:reinterpret_cast<cfg::node*>(n3)->get_nodes()){
                            out << gen_one_key_string(name+"|"+n2->get_name()+"|"+n3->get_name(), n4,test_ty);
                        }
                    }
                }
            }
        }
        out<<"};\n";
        return out.str();
    };

    auto gen_node_key_array=[&]()->std::string{

        std::ostringstream out;
        out<<"final String[] NODE_KEYS={\n";
        for(auto n:g_cfg.get_nodes()){
            const std::string& name=n->get_name();
            if(gen_is_parent(name)){

                out<<"\""<<name<<"\",\n";

                for(auto n2:reinterpret_cast<cfg::node*>(n)->get_nodes()){
                    if(std::find(std::begin(gen_skips),std::end(gen_skips),name+"|"+n2->get_name())!=std::end(gen_skips))
                        continue;

                    if(n2->get_type()==cfg::type::node){
                        out<<"\""<<name+"|"+n2->get_name()<<"\",\n";

                        for(auto n3:reinterpret_cast<cfg::node*>(n2)->get_nodes()) {

                            if(std::find(std::begin(gen_skips),std::end(gen_skips),name+"|"+n2->get_name()+"|"+n3->get_name()))
                                continue;

                            if(n3->get_type()==cfg::type::node){
                                out<<"\""<<name+"|"+n2->get_name()+"|"+n3->get_name()<<"\",\n";
                            }
                        }
                    }
                }
            }
        }
        out<<"};\n";
        return out.str();
    };

    std::ostringstream out;

    out<<gen_one_key_array("final String[] BOOL_KEYS={",cfg::type::_bool);
    out<<gen_one_key_array("final String[] INT_KEYS={",cfg::type::_int);
    out<<gen_one_key_array("final String[] INT_KEYS={",cfg::type::uint);
    out<<gen_one_key_array("final String[] STRING_ARR_KEYS={",cfg::type::_enum);

    //
    out<<gen_node_key_array();

    return env->NewStringUTF(out.str().c_str());
}

static jobjectArray j_get_native_llvm_cpu_list(JNIEnv* env,jobject self){

    const std::vector<core_info_t> core_info_list = cpu_get_core_info();
    const std::set<std::string,std::greater<>> llvm_cpu_list=get_processor_name_set(core_info_list);

    int count=llvm_cpu_list.size();

    jobjectArray ret=env->NewObjectArray(count,env->FindClass("java/lang/String"),nullptr);
    int n=0;
    for(const std::string& cpu_name:llvm_cpu_list){
        env->SetObjectArrayElement(ret,n++,env->NewStringUTF(cpu_name.c_str()));
    }
    return ret;
}

static jobjectArray j_get_support_llvm_cpu_list(JNIEnv* env,jobject self){

    const std::vector<core_info_t> core_info_list = cpu_get_core_info();
    const std::set<std::string,std::greater<>> llvm_cpu_list=get_processor_name_set(core_info_list);
    const std::string isa=cpu_get_processor_isa(core_info_list[0]);
    std::vector<std::string> append_llvm_cpu_list;
    int count=llvm_cpu_list.size();
    if(isa=="armv9-a"||isa=="armv9.2-a"){
        count+=4;
        append_llvm_cpu_list={"cortex-x1","cortex-a55","cortex-a73","cortex-a53"};
    }
    else if(isa=="armv8.2-a"){
        count+=2;
        append_llvm_cpu_list={"cortex-a73","cortex-a53"};
    }

    jobjectArray ret=env->NewObjectArray(count,env->FindClass("java/lang/String"),nullptr);
    int n=0;
    for(const std::string& cpu_name:llvm_cpu_list){
        env->SetObjectArrayElement(ret,n++,env->NewStringUTF(cpu_name.c_str()));
    }
    for(const std::string& cpu_name:append_llvm_cpu_list){
        env->SetObjectArrayElement(ret,n++,env->NewStringUTF(cpu_name.c_str()));
    }
    return ret;
}

static jobjectArray j_get_vulkan_physical_dev_list(JNIEnv* env,jobject self){

    std::vector<VkPhysicalDeviceProperties> physical_device_prop_list;
    {
        VkApplicationInfo appinfo = {};
        appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appinfo.pNext = nullptr;
        appinfo.pApplicationName = "aps3e-cfg-test";
        appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appinfo.pEngineName = "nul";
        appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appinfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo inst_create_info = {};
        inst_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_create_info.pApplicationInfo = &appinfo;

        VkInstance inst;
        if (vkCreateInstance(&inst_create_info, nullptr, &inst)!= VK_SUCCESS) {
            __android_log_print(ANDROID_LOG_FATAL, LOG_TAG,"%s : %d",__func__,__LINE__);
        }

        // 获取物理设备数量
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, nullptr);

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, physicalDevices.data());

        for (const auto& physicalDevice : physicalDevices) {
            VkPhysicalDeviceProperties physicalDeviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
            physical_device_prop_list.push_back(physicalDeviceProperties);
        }

        vkDestroyInstance(inst, nullptr);
    }
    int count=physical_device_prop_list.size();
    jobjectArray ret=env->NewObjectArray(count,env->FindClass("java/lang/String"),nullptr);

    int n=0;
    for(const VkPhysicalDeviceProperties& prop:physical_device_prop_list){
        env->SetObjectArrayElement(ret,n++,env->NewStringUTF(prop.deviceName));
    }
    return ret;
}

/*
static jboolean support_custom_driver(JNIEnv* env,jobject self){
    return access("/dev/kgsl-3d0",F_OK)==0;
}*/

int register_aps3e_Emulator(JNIEnv* env){

    static const JNINativeMethod methods[] = {

            {"get_cpu_core_count", "()I",(void *)j_get_cpu_core_count},
            {"get_cpu_name", "(I)Ljava/lang/String;",(void *)j_get_cpu_name},
            {"get_cpu_max_mhz", "(I)I",(void *)j_get_cpu_max_mhz},
            {"simple_device_info", "()Ljava/lang/String;",(void *)j_simple_device_info},
            {"get_native_llvm_cpu_list", "()[Ljava/lang/String;",(void *)j_get_native_llvm_cpu_list},
            {"get_support_llvm_cpu_list", "()[Ljava/lang/String;",(void *)j_get_support_llvm_cpu_list},
            {"get_vulkan_physical_dev_list", "()[Ljava/lang/String;",(void *)j_get_vulkan_physical_dev_list},

            //{"support_custom_driver","()Z",(void *)support_custom_driver},

            { "generate_config_xml", "()Ljava/lang/String;", (void *) generate_config_xml },
            {"generate_strings_xml","()Ljava/lang/String;", (void *)generate_strings_xml},
            {"generate_java_string_arr","()Ljava/lang/String;", (void *)generate_java_string_arr},

            { "install_firmware", "(I)Z", (void *) j_install_firmware },
            //{ "meta_info_from_iso","(Ljava/lang/String;)Laenu/aps3e/Emulator$MetaInfo;",(void*)MetaInfo_from_iso},
            { "meta_info_from_dir","(Ljava/lang/String;)Laenu/aps3e/Emulator$MetaInfo;",(void*)j_meta_info_from_dir},

            { "meta_info_from_iso","(ILjava/lang/String;)Laenu/aps3e/Emulator$MetaInfo;",(void*)j_meta_info_from_iso},

            { "setup_game_id", "(Ljava/lang/String;)V", (void *) j_setup_game_id },
            {"install_edat", "(I)Z", (void *) j_install_edat},
            { "install_pkg", "(I)Z", (void *) j_install_pkg },

    };

    jclass clazz = env->FindClass("aenu/aps3e/Emulator");
    return env->RegisterNatives(clazz,methods, sizeof(methods)/sizeof(methods[0]));

}