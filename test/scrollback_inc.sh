export PS1="$ "
export PROMPT_COMMAND=

printf "\e[H\e[J\n"

# saveLines == 500; plus 23 lines to fill, plus one line of prompt
for x in {1..523} ; do
    printf "%3d |%*s\n" $x $((1 + x/7)) "*"
done
