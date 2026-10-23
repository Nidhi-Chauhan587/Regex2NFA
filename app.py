import gradio as gr
import subprocess
import re
import os
from graphviz import Digraph

def generate_nfa(regex):
    regex = regex.strip()

    if not regex:
        return "Please enter a regular expression.", None

    try:
        result = subprocess.run(
            ["./main"],
            input=regex + "\n",
            capture_output=True,
            text=True,
            timeout=10
        )

        output = result.stdout + result.stderr

        if result.returncode != 0:
            return output, None

        start_match = re.search(r"Start state:\s*q(\d+)", output)
        final_match = re.search(r"Accepting state:\s*q(\d+)", output)
        count_match = re.search(r"Total states:\s*(\d+)", output)

        if not start_match or not final_match or not count_match:
            return output + "\nUnable to read NFA details.", None

        start_state = start_match.group(1)
        final_state = final_match.group(1)
        total_states = int(count_match.group(1))

        diagram = Digraph(format="png")
        diagram.attr(
            rankdir="LR",
            label="NFA - Thompson's Construction",
            labelloc="t",
            fontsize="18",
            bgcolor="white",
            pad="0.3"
        )

        for i in range(total_states):
            state = str(i)

            if state == final_state:
                diagram.node(
                    state, "q" + state,
                    shape="doublecircle",
                    style="filled",
                    fillcolor="lightgreen"
                )
            else:
                diagram.node(
                    state, "q" + state,
                    shape="circle",
                    style="filled",
                    fillcolor="lightblue"
                )

        diagram.node("start_arrow", "", shape="point")
        diagram.edge("start_arrow", start_state)

        pattern = r"^\s*q(\d+)\s+(\S+)\s+q(\d+)\s*$"

        for line in output.splitlines():
            match = re.match(pattern, line)

            if match:
                source, symbol, target = match.groups()

                if symbol.lower() == "epsilon":
                    symbol = "ε"

                diagram.edge(source, target, label=symbol)

        image_path = diagram.render(
            filename="/tmp/nfa_diagram",
            cleanup=True
        )

        return output, image_path

    except subprocess.TimeoutExpired:
        return "The program took too long to respond.", None
    except Exception as error:
        return f"Error: {error}", None

with gr.Blocks(title="Regex to NFA Converter") as app:
    gr.Markdown("# Regex to NFA Converter")
    gr.Markdown(
        "Regular Expression to NFA using Thompson's Construction"
    )

    regex_input = gr.Textbox(
        label="Enter Regular Expression",
        placeholder="Example: a(b|c)*d",
        value="a(b|c)*d"
    )

    generate_button = gr.Button(
        "Generate NFA",
        variant="primary"
    )

    output = gr.Textbox(
        label="NFA Details and Transition Table",
        lines=18,
        interactive=False
    )

    diagram_output = gr.Image(
        label="Visual NFA State Diagram",
        type="filepath"
    )

    generate_button.click(
        fn=generate_nfa,
        inputs=regex_input,
        outputs=[output, diagram_output]
    )

if __name__ == "__main__":
    app.launch(
        server_name="0.0.0.0",
        server_port=int(os.environ.get("PORT", "7860"))
    )
