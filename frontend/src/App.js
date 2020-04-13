import React, { useState, useEffect } from 'react';
import clsx from 'clsx';
import Container from '@material-ui/core/Container';
import Grid from '@material-ui/core/Grid';
import Paper from '@material-ui/core/Paper';
import { makeStyles, createStyles } from '@material-ui/core/styles';
import axios from 'axios';


const useStyles = makeStyles((theme) => createStyles({
  content: {
    flexGrow: 1,
    height: '100vh',
    overflow: 'auto',
  },
  container: {
    paddingTop: theme.spacing(4),
    paddingBottom: theme.spacing(4),
  },
  paper: {
    padding: theme.spacing(2),
    display: 'flex',
    overflow: 'auto',
    flexDirection: 'column',
  },
  fixedHeight: {
    height: 240,
  },
}));

const updateSensorReadings = (cb) =>
  axios.get('/api/v1/sensors')
    .then(res => {
      cb(res.data);
    })
    .catch(e => {
      console.log(e);
    });

function App() {
  const classes = useStyles();
  const [sensors, setSensors] = useState({});

  useEffect(() => {
    updateSensorReadings(setSensors);
    const intervalHandle = setInterval(() => updateSensorReadings(setSensors), 10000);
    return () => clearInterval(intervalHandle);
  }, []);

  const fixedHeightPaper = clsx(classes.paper, classes.fixedHeight);

  return (
    <main className={classes.content}>
      <div className={classes.appBarSpacer} />
      <Container maxWidth="lg" className={classes.container}>
        <Grid container spacing={3}>
          <Grid item xs={12} sm={6} lg={4}>
            <Paper className={fixedHeightPaper}>
              Temperature: {sensors.temperature}
            </Paper>
          </Grid>
          <Grid item xs={12} sm={6} lg={4}>
            <Paper className={fixedHeightPaper}>
              Humidity: {sensors.humidity}
            </Paper>
          </Grid>
          <Grid item xs={12} sm={6} lg={4}>
            <Paper className={fixedHeightPaper}>
              Luminescence: {sensors.luminescence}
            </Paper>
          </Grid>
          <Grid item xs={12} sm={6} lg={4}>
            <Paper className={fixedHeightPaper}>
              Moisture: {sensors.moisture}
            </Paper>
          </Grid>
          <Grid item xs={12} sm={6} lg={4}>
            <Paper className={fixedHeightPaper}>
              Light: {sensors.light}
            </Paper>
          </Grid>
        </Grid>
      </Container>
    </main>
  );
}

export default App;
