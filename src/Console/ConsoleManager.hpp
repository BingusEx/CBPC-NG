#pragma once 

namespace CBP {

    class ConsoleManager : public CInitSingleton<ConsoleManager> {

		private:
        struct Command {
            std::function<void()> callback = nullptr;
            std::string desc;
            explicit Command(const std::function<void()>& callback, std::string desc) : callback(callback), desc(std::move(desc)) {}
        };

        //default base command preffix
        inline static const std::string Default_Preffix = "cbp";
        static inline absl::flat_hash_map<std::string, Command> RegisteredCommands = {};

		public:
        static void Init();
        static void RegisterCommand(std::string_view a_cmdName, const std::function<void()>& a_callback, const std::string& a_desc);
        static bool Process(const std::string& a_msg);
        static void CMD_Help();
    };
}