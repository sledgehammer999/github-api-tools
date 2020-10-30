# Helper function for coupling add_feature_info() and option()

function(feature_option _name _description _default)
    string(CONCAT _desc "${_description} (default: ${_default})")
    option("${_name}" "${_desc}" "${_default}")
    add_feature_info("${_name}" "${_name}" "${_desc}")
endfunction()
