function atdm_match_keyword() {
  input_string=$1
  keyword=$2

  lower_input_string=$(echo "$input_string" | tr '[:upper:]' '[:lower:]')
  lower_keyword=$(echo "$keyword" | tr '[:upper:]' '[:lower:]')

  #echo "lower_input_string='${lower_input_string}'"
  #echo "lower_keyword='${lower_keyword}'"

  if   [[ "${lower_input_string}" == "${lower_keyword}" ]] ; then
    return 0
  elif [[ "${lower_input_string}" == *"-${lower_keyword}" ]] ; then
    return 0
  elif [[ "${lower_input_string}" == *"_${lower_keyword}" ]] ; then
    return 0
  elif [[ "${lower_input_string}" == "${lower_keyword}-"* ]] ; then
    return 0
  elif [[ "${lower_input_string}" == "${lower_keyword}_"* ]] ; then
    return 0
  elif [[ "${lower_input_string}" == *"-${lower_keyword}-"* ]] ; then
    return 0
  elif [[ "${lower_input_string}" == *"-${lower_keyword}_"* ]] ; then
    return 0
  elif [[ "${lower_input_string}" == *"_${lower_keyword}-"* ]] ; then
    return 0
  elif [[ "${lower_input_string}" == *"_${lower_keyword}_"* ]] ; then
    return 0
  fi

  return 1

}
