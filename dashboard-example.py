#%%
import dash
import dash_core_components as dcc
import dash_html_components as html
import numpy as np
import pandas as pd
from dash.dependencies import Input, Output

# Crie uma aplicação Dash
app = dash.Dash(__name__)

# Gerando dados aleatórios
np.random.seed(42)
dates = pd.date_range(start="2024-01-01", periods=100)
values = np.random.randn(100).cumsum()

# DataFrame com os dados
df = pd.DataFrame({"Date": dates, "Value": values})

# Layout do dashboard
app.layout = html.Div([
    html.H1("Dashboard Simples com Dash"),
    
    dcc.Graph(id="line-chart"),
    
    dcc.Slider(
        id="date-slider",
        min=0,
        max=len(df) - 1,
        value=len(df) - 1,
        marks={i: str(date.date()) for i, date in enumerate(df['Date'])},
        step=None
    )
])

# Callback para atualizar o gráfico
@app.callback(
    Output("line-chart", "figure"),
    [Input("date-slider", "value")]
)
def update_chart(selected_date_index):
    filtered_df = df.iloc[:selected_date_index + 1]
    
    return {
        "data": [{
            "x": filtered_df["Date"],
            "y": filtered_df["Value"],
            "type": "line"
        }],
        "layout": {
            "title": "Gráfico de Linha",
            "xaxis": {"title": "Data"},
            "yaxis": {"title": "Valor"},
        }
    }

# Executando a aplicação
if __name__ == "__main__":
    app.run_server(debug=True)
