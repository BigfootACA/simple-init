find_package(Gettext REQUIRED)
set(GETTEXT_PACKAGE "simple-init")
foreach(lang zh_CN)
	gettext_process_po_files(${lang} ALL PO_FILES po/${lang}.po)
endforeach()

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/root/usr/share/locale"
	COMMAND bash
		"${CMAKE_CURRENT_SOURCE_DIR}/scripts/install-gmo.sh"
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_BINARY_DIR}"
		"${CMAKE_SOURCE_DIR}/root"
	DEPENDS pofiles
)
