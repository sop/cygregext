#include "registry.hpp"
#include "app.hpp"
#include "util/strconv.hpp"
#include "util/winerror.hpp"
#include "util/shutil.hpp"

namespace cygextreg
{

void Registry::registerExtension(const std::wstring& ext,
                                 const std::wstring& icon,
                                 const Settings& settings) {
	Key base = _getClassBase();
	std::wstring handler_name = _handlerPrefix + ext;
	std::wstring desc = std::wstring(L"Cygwin Shell Script (" + ext + L")");
	std::wstring cmd = _getOpenCommand(settings);
	/* remove previous */
	if (base.hasSubKey(handler_name)) {
		base.deleteSubTree(handler_name);
	}
	/* handler key */
	Key handler = Key::create(base, handler_name)
	                  .setString(std::wstring(), desc)
	                  .setDword(L"EditFlags", 0x30)
	                  .setString(L"FriendlyTypeName", desc);
	/* default icon */
	Key::create(handler, L"DefaultIcon")
	    .setString(std::wstring(), icon);
	/* open command */
	Key shell = Key::create(handler, L"shell")
	                .setString(std::wstring(), L"open");
	Key open = Key::create(shell, L"open")
	               .setString(std::wstring(), L"Run in Cygwin")
	               .setString(L"Icon", icon);
	Key::create(open, L"command")
	    .setString(std::wstring(), cmd);
	/* runas verb (run as administrator) */
	Key runas = Key::create(shell, L"runas")
	                .setString(L"Icon", icon)
	                .setString(L"Extended", L"");
	Key::create(runas, L"command")
	    .setString(std::wstring(), cmd);
	/* drop handler */
	Key::create(handler, L"shellex\\DropHandler")
	    .setString(std::wstring(), L"{86C86720-42A0-1069-A2E8-08002B30309D}");
	/* extension key */
	Key::create(base, ext)
	    .setString(std::wstring(), handler_name);
}

void Registry::unregisterExtension(const std::wstring& ext) {
	Key base = _getClassBase();
	std::wstring handler_name = _handlerPrefix + ext;
	if (!base.hasSubKey(handler_name)) {
		throw std::runtime_error(std::string("Extension ") +
		                         wide_to_mb(ext) + " is not registered.");
	}
	base.deleteSubTree(handler_name);
	base.deleteSubTree(ext);
}

std::vector<std::wstring> Registry::searchRegisteredExtensions() {
	/* handler prefix to search for */
	std::wstring _prefix = _handlerPrefix + L".";
	Key base = _getClassBase(KEY_READ);
	wchar_t name[256];
	DWORD name_size;
	std::vector<std::wstring> handlers;
	for(DWORD idx = 0;; ++idx) {
		name_size = sizeof(name) / sizeof(name[0]);
		LONG result = RegEnumKeyEx(
			base, idx, name, &name_size, NULL, NULL, NULL, NULL);
		if (ERROR_NO_MORE_ITEMS == result) {
			break;
		}
		else if (ERROR_MORE_DATA == result) {
			/* silently skip entries that don't fit to buffer,
			   we're looking for entries of ~12 characters anyway */
			continue;
		}
		else if (ERROR_SUCCESS != result) {
			THROW_ERROR_CODE("Failed to enumerate registry", result);
		}
		/* if key name starts with prefix */
		if (0 == wcsncmp(name, _prefix.c_str(), _prefix.size())) {
			handlers.push_back(name);
		}
	}
	std::vector<std::wstring> exts;
	for (auto handler : handlers) {
		std::wstring ext = handler.substr(9);
		if (base.hasSubKey(ext) && !isRegisteredForOther(ext)) {
			exts.push_back(ext);
		}
	}
	return exts;
}

bool Registry::isRegisteredForOther(const std::wstring& ext) {
	Key base = _getClassBase(KEY_QUERY_VALUE);
	/* if extension is not registered at all */
	if (!base.hasSubKey(ext)) {
		return false;
	}
	Key ext_key(base, ext, KEY_QUERY_VALUE);
	/* if extension doesn't have this app as a handler */
	std::wstring handler_name = _handlerPrefix + ext;
	if (ext_key.getString(L"") != handler_name) {
		return true;
	}
	/* get handler's open command program */
	std::wstring command;
	try {
		command = Key(base, handler_name + L"\\shell\\open\\command",
		              KEY_QUERY_VALUE).getString(L"");
	} catch(...) {
		return true;
	}
	/* if there's no open command */
	if (!command.size()) {
		return true;
	}
	int argc;
	LPWSTR* argv = CommandLineToArgvW(command.c_str(), &argc);
	if (NULL == argv || argc < 1) {
		return true;
	}
	std::wstring prog(argv[0]);
	/* if open command differs from this instance */
	if (App::getPath().longPath().str() != prog) {
		return true;
	}
	return false;
}

Key Registry::_getClassBase(REGSAM access) {
	return Key(*_rootKey, L"Software\\Classes", access);
}

std::wstring Registry::_getOpenCommand(const Settings& settings) {
	std::wstring path = App::getPath().longPath();
	std::wstringstream ss;
	ss << escapeWinArg(path)
	   << L" --on_exit "
	   << escapeWinArg(mb_to_wide(settings.exitBehaviourStr()))
	   << L" --exec -- " << escapeWinArg(L"%1") << L" %*";
	return ss.str();
}

}
